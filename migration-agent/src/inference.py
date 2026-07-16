"""
This defines the routines for using LLMs for inference
"""

from pathlib import Path
import json
import demjson3
import logging
from json import JSONDecoder
from utilities.llms import LLM
from utilities.utilities import pretty_print_code, pretty_print_dialog
from global_config import *
import re
import os


class LLMRepair:

    def __init__(self):
        self.llm = LLM()
        
        # 定义JSON Schema
        self.json_schema = {
            "type": "object",
            "required": ["code_changes", "compiler_options", "reasoning"],
            "properties": {
                "code_changes": {
                    "type": "object",
                    "description": "修改后的源代码，键为文件路径，值为完整代码"
                },
                "compiler_options": {
                    "type": "string",
                    "description": "编译选项，可以是字符串或字符串数组"
                },
                "reasoning": {
                    "type": "string",
                    "description": "修改原因的简要说明"
                }
            },
            "additionalProperties": False
        }
        
        # 改进的修复提示模板
                # Improved repair prompt template with strict format enforcement
                
        self.repair_prompt_template = (
            "You are a senior C/C++ compilation error repair expert. Return the solution STRICTLY in the following JSON format:\n\n"
            f"JSON SCHEMA: {json.dumps(self.json_schema, indent=2)}\n\n"
            "CRITICAL REQUIREMENTS:\n"
            "1. Return ONLY a single valid JSON object - NO preliminary text, explanations, or trailing content\n"
            "2. JSON must start with '{' and end with '}' with NO leading/trailing whitespace\n"
            "3. Escape all special characters in strings (e.g., use \\\" for internal quotes)\n"
            "4. For code fixes: Provide COMPLETE modified source code (not snippets) in 'code_changes'\n"
            "5. For compiler option fixes: Provide the FULL corrected command in 'compiler_options'\n"
            "6. Omit all diagnostic/thinking processes - output ONLY the JSON payload\n\n"
            # "7. STRICTLY PROHIBITED: Any form of delimiters (e.g., ```, ===, </think>) or markers around the JSON\n"  
            # "8. OUTPUT MUST CONTAIN ONLY JSON - ANY NON-JSON CONTENT WILL RESULT IN TOTAL REJECTION\n"  
            "Based on the following compilation error, compiler command, and source code, provide the definitive repair solution:"
        )
        
        self.template_code_suffix = "The complete code from [SOURCE_FILE] is as follows: ``` [CODE] ```, "
        self.repair_prompt_template_suffix = (
            "The exact compiler command used is: ``` [COMMAND] ```\n"
            "The complete compilation error output is: ``` [ERROR] ```"
        )
        
        # Improved source file extraction prompt with technical specificity
        self.extract_prompt_template = (
            "As a compiler diagnostics expert, analyze the compilation error to determine if additional source files must be examined.\n"
            "Return STRICTLY a JSON object in THIS format (NO explanations outside JSON):\n\n"
            "{\n"
            '  "source_file": "filename to inspect (or null if no additional files needed)",\n'
            '  "reasoning": "concise technical justification (max 50 words) for needing or excluding the file"\n'
            "}\n\n"
            "JUSTIFICATION GUIDELINES:\n"
            "- Reference specific error tokens/line numbers when possible\n"
            "- Explain dependency relationships (e.g., 'error references type defined in header')\n"
            "- Use 'null' ONLY when error is fully contained in provided files"
        )
        
        
        self.extract_prompt_template_suffix = (
            "The compiler command executed was: ``` [COMMAND] ```\n"
            "The complete error output from compilation is: ``` [ERROR] ```, "
        )

    # Get prompt template
    def get_repair_prompt_template(self):
        return self.repair_prompt_template

    # Return the model id (name) used for inference
    def get_model_id(self):
        return self.llm.get_model_id()

    def extract_json_objects(self, text, decoder=JSONDecoder()):
        """提取文本中的JSON对象"""
        pos = 0
        while True:
            match = text.find("{", pos)
            if match == -1:
                break
            try:
                result, index = decoder.raw_decode(text[match:])
                yield result
                pos = match + index
            except ValueError:
                pos = match + 1

    def _extract_code_changes(self, code_changes):
        """从LLM响应中提取并记录代码更改"""
        code = {}
        if code_changes:
            for source, code_text in code_changes.items():
                # 将列表转换为字符串（按行拼接）
                if isinstance(code_text, list):
                    code_text = '\n'.join(map(str, code_text))
                code[source] = code_text
                logging.debug(
                    pretty_print_code(f"LLM Generated Code for {source}", code_text)
                )
        return code

    def _extract_compiler_options(self, data):
        """从LLM响应中提取编译选项"""
        compiler_options = data.get("compiler_options")
        if compiler_options:
            # 处理数组类型的编译选项
            if isinstance(compiler_options, list):
                compiler_options = " ".join(str(option) for option in compiler_options)
                logging.warning("编译选项返回为数组类型，已自动转换为字符串")
            
            logging.debug(
                pretty_print_dialog("LLM suggested options", compiler_options)
            )
        return compiler_options

    def _extract_reasoning(self, data, reasoning_content):
        """从LLM响应中提取推理说明"""
        reasoning = data.get("reasoning")
        return reasoning if reasoning else reasoning_content

    def _extract_source_target(self, data):
        """从LLM响应中提取所需的源文件"""
        return data.get("source_file")

    def process_response(self, message_content, reasoning_content, max_retries=2):
        """处理LLM响应，确保有效的JSON格式"""
        # 记录原始响应用于调试
        logging.debug(f"\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
                      f"+ LLM inference raw data +\n+ {message_content} +\n"
                      f"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n")
        
        # 清理响应内容
        def clean_json_content(content):
            # 移除Markdown代码块标记
            content = re.sub(r'<think>.*?</think>', '', content, flags=re.DOTALL)
            content = re.sub(r'```(?:json)?\s*|\s*```', '', content, flags=re.DOTALL)
            
            # 查找第一个 { 和最后一个 } 之间的内容
            start_idx = content.find('{')
            end_idx = content.rfind('}')
            
            if start_idx != -1 and end_idx > start_idx:
                return content[start_idx:end_idx+1]
            return content
        
        # 清理消息内容
        cleaned_content = clean_json_content(message_content)
        logging.debug(f"Cleaned JSON content: {cleaned_content}")
        
        try:
            # 尝试解析JSON
            data = demjson3.decode(cleaned_content)
            
        except Exception as e:
            logging.debug(f"Failed to parse json data {cleaned_content} {str(e)}")
            
            # 检查重试次数
            if max_retries <= 0:
                logging.warning("Max retries reached for JSON correction")
                return {}
            
            # 准备重试提示
            retry_prompt = (
                f"你的上一个JSON响应无效，解析错误: {str(e)}\n"
                f"问题内容开头: {cleaned_content[:50]}...\n\n"
                "请提供一个严格有效的JSON对象，必须满足以下要求:\n"
                "1. JSON必须以'{'开始，以'}'结束\n"
                "2. 不能有任何前导字符或注释\n"
                "3. 所有字符串必须正确转义(例如: 使用\\\"表示内部引号)\n"
                "4. 不要在JSON外包含任何文本\n"
                "5. 不要输出任何思考过程\n\n"
                f"JSON SCHEMA: {json.dumps(self.json_schema, indent=2)}\n\n"
                "仅返回修正后的JSON，不要有任何额外内容。"
            )
            
            # 递归调用减少重试次数
            return self.inference(
                prompt=retry_prompt, 
                task_msg="to correct invalid JSON",
                max_retries=max_retries - 1
            )

        # 提取并返回结果
        res = {
            "code": self._extract_code_changes(data.get("code_changes", {})),
            "compiler_options": self._extract_compiler_options(data),
            "reasoning": self._extract_reasoning(data, reasoning_content),
            "source_file": self._extract_source_target(data),
        }

        return res

    def populate_prompt_template(self, template: str, command: str, error: str) -> str:
        """填充提示模板中的命令和错误信息"""
        return template.replace("[COMMAND]", command).replace("[ERROR]", error)

    def populate_code_to_prompt(self, prompt, code: dict):
        """将代码添加到提示中"""
        if code:
            for file, c in code.items():
                prompt += self.template_code_suffix.replace(
                    "[SOURCE_FILE]", file
                ).replace("[CODE]", c)
        return prompt

    def query_llm_for_fix(self, code: dict, command, error, prompt_template=None, model_id=None):
        """Query LLM for error repair solution based on code, command and error"""
        prompt = prompt_template or self.repair_prompt_template
        prompt = self.populate_code_to_prompt(prompt=prompt, code=code)
        prompt += self.repair_prompt_template_suffix
        
        # Add final format enforcement
        prompt += "\n\nFINAL VALIDATION: Your response must parse as valid JSON using standard JSON parsers. Any deviation will be rejected."
        
        return self.inference(
            prompt=self.populate_prompt_template(prompt, command, error),
            model_id=model_id,
            task_msg="to repair compilation errors.",
        )
    def query_llm_for_source_file(
        self, code: dict, command, error, prompt_template=None, model_id=None
    ):
        """查询LLM确定解决问题所需的源文件"""
        prompt = prompt_template or self.extract_prompt_template
        prompt += self.extract_prompt_template_suffix
        prompt = self.populate_code_to_prompt(prompt=prompt, code=code)
        
        # 添加强调不要输出思考过程的指令
        prompt += "\n\n重要说明: 只返回JSON结果，不要输出思考过程。"
        
        return self.inference(
            prompt=self.populate_prompt_template(prompt, command, error),
            model_id=model_id,
            task_msg="to locate relevant source files.",
        )

    def inference(self, prompt, model_id=None, task_msg="", max_retries=2):
        """通过查询LLM执行推理"""
        prev_model = self.llm.get_model_id()
        if model_id:
            self.llm.set_model_id(model_id)
        else:
            model_id = self.llm.get_model_id()

        # 添加格式控制指令
        format_instruction = (
            "\n\nNON-NEGOTIABLE FORMAT REQUIREMENTS:\n"
            "1. Output contains EXACTLY ONE JSON object\n"
            "2. JSON must validate against RFC 8259 standards\n"
            "3. No comments, markdown, or explanatory text\n"
            "4. String values must use proper escape sequences\n"
            "5. Ensure trailing braces are present and properly placed"
        )
        enhanced_prompt = prompt + format_instruction

        logging.info(f"Calling {model_id} {task_msg}")
        response = self.llm.inference(enhanced_prompt)

        if response:
            message_content = response.get("content", "")
            reasoning_content = response.get("reasoning_content", "")
            self.llm.set_model_id(prev_model)
            return self.process_response(message_content, reasoning_content, max_retries=max_retries)

        logging.error("Failed in getting the inference response")
        self.llm.set_model_id(prev_model)
        return {}