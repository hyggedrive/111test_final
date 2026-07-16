""" This defines LLM related utility functions """

import json
import requests
import logging
from global_config import *
# from llama_cpp import Llama

# Suppress all warnings for cleaner output
import warnings

warnings.filterwarnings("ignore")


# class ModelType:
#     OPENAI = "openai"
#     LOCAL_OLLAMA = "local_ollama"
#     LLAMA_CPP_CPU = "llama_cpp_cpu"  # llama.cpp CPU部署


class LLM:
    def __init__(self):
        """Initialize LLM instance with the model ID and API headers."""
        self.headers = {
            "Authorization": self.get_api_token(),
            "Content-Type": "application/json",
        }
        self.model_id = get_model_id()

    def get_api_token(self) -> str:
        """Return the API token required for authorization."""
        return f"Bearer {llm_api_token}"

    def get_model_id(self):
        """Return the current LLM model ID."""
        return self.model_id

    def set_model_id(self, model_id: str):
        """Set the model ID for the LLM."""
        self.model_id = model_id

    def inference(self, prompt):
        model_type = get_model_type()
        if model_type == ModelType.OPENAI:
            return self.openapi_inference(prompt)
        elif model_type == ModelType.LOCAL_OLLAMA:
            return self.local_ollama_inference(prompt)
        # elif model_type == ModelType.LLAMA_CPP_CPU:
        #     return self.local_llamacpp_inference(prompt)
        else:
            logging.error(f"Unsupported model type: {model_type}")
            return {"content": "", "reasoning_content": ""}

    def openapi_inference(self, prompt):
        payload = {
            "model": self.model_id,
            "messages": [
                {
                    "role": "user",
                    "content": prompt,
                }
            ],
            "temperature": 0.3,
            "stream": get_enable_stream(),
        }

        try:
            response = requests.post(
                get_llm_url(),
                json=payload,
                headers=self.headers,
                verify=False,
                stream=get_enable_stream()
            )
            response.raise_for_status()

            if get_enable_stream():
                content, reasoning_content = self._process_openai_stream(response)
            else:
                content, reasoning_content = self._process_openai_non_stream(response)
            return {"content": content, "reasoning_content": reasoning_content}
        except requests.exceptions.RequestException as e:
            logging.error(f"OpenAI request failed: {e}")
            return {"content": "", "reasoning_content": ""}

    def _process_openai_stream(self, response):
        full_content = ""
        reasoning_content = ""

        for line in response.iter_lines():
            if not line:
                continue

            # 解码并清理响应行，移除 'data: ' 前缀
            decoded_line = line.decode('utf-8').replace("data: ", "")

            # 判断是否为流式响应的结束标记
            if decoded_line == "[DONE]":
                break

            try:
                data = json.loads(decoded_line)
                content_part = ""
                reasoning_part = ""
                # 从 JSON 数据中提取 content 和 reasoning_content
                if 'choices' in data and data['choices']:
                    delta = data['choices'][0].get('delta', {})
                    content_part = delta.get('content', '')
                    if content_part is None:
                        content_part = ""
                    reasoning_part = delta.get('reasoning_content')
                    if reasoning_part is None:
                        reasoning_part = ""
                full_content += content_part
                reasoning_content += reasoning_part
            except json.JSONDecodeError:
                logging.debug("Failed to decode JSON line in stream response")
                continue

        return full_content, reasoning_content

    def _process_openai_non_stream(self, response):
        content = ""
        reasoning_content = ""
        try:
            data = response.json()
            if 'choices' in data and data['choices']:
                content = data['choices'][0]['message']['content']
                reasoning_content = data['choices'][0]['message'].get('reasoning_content', '')
        except (KeyError, IndexError, ValueError):
            logging.error("Failed to parse OpenAI response JSON")
        return content, reasoning_content

    def local_ollama_inference(self, prompt):
        payload = {
            "model": self.model_id,
            "prompt": prompt,
            "options": {
                "temperature": 0.3
            },
            "stream": get_enable_stream()
        }

        try:
            response = requests.post(
                get_llm_url(),
                json=payload,
                headers=self.headers,
                verify=False,
                stream=get_enable_stream(),
                timeout=30
            )
            response.raise_for_status()

            if get_enable_stream():
                content, reasoning_content = self._process_ollama_stream(response)
            else:
                content, reasoning_content = self._process_ollama_non_stream(response)
            return {"content": content, "reasoning_content": reasoning_content}
        except requests.exceptions.RequestException as e:
            logging.error(f"Ollama request failed: {e}")
            return {"content": "", "reasoning_content": ""}

    def _process_ollama_stream(self, response):
        full_content = ""
        reasoning_content = ""
        for line in response.iter_lines():
            if line:
                try:
                    line_decoded = line.decode('utf-8')
                    data = json.loads(line_decoded)
                    response_text = data.get('response', '')
                    if response_text is None:
                        response_text = ""
                    full_content += response_text
                    reasoning_delta = data.get('reasoning_content', '')
                    if reasoning_delta is None:
                        reasoning_delta = ""
                    reasoning_content += reasoning_delta
                    
                    if data.get('done'):
                        break
                except json.JSONDecodeError as e:
                    logging.debug(f"Failed to decode JSON: {e}")
                    continue
    
        # 处理完整内容，移除 <think>...</think> 部分
        import re
        think_pattern = re.compile(r'<think>.*?</think>', re.DOTALL)
        full_content = think_pattern.sub('', full_content).strip()
    
        # 记录清理后的内容供调试
        logging.debug(f"Processed content: {full_content[:200]}...")
    
        return full_content, reasoning_content

    def _process_ollama_non_stream(self, response):
        content = ""
        reasoning_content = ""
        try:
            # 检查 response 是否已经是字典
            if isinstance(response, dict):
                data = response
            else:
                data = response.json()
            
            # 获取响应内容
            content = data.get('response', '') or data.get('content', '')
            
            # 移除 <think>...</think> 部分
            import re
            think_pattern = re.compile(r'<think>.*?</think>', re.DOTALL)
            content = think_pattern.sub('', content).strip()
            
            # 获取推理内容
            reasoning_content = data.get('reasoning_content', '')
        except Exception as e:
            logging.error(f"Failed to parse Ollama response JSON: {str(e)}")
        return content, reasoning_content

    # def local_llamacpp_inference(self, prompt):
    #     """使用本地 LlamaCPP 模型进行推理"""
    #     try:
           
            
    #         # 从环境变量获取模型路径
    #         model_path = get_llamacpp_model_path()
            
    #         # 硬编码三个参数
    #         n_ctx = 4096      # 上下文窗口大小硬编码为4096
    #         n_threads = 4     # 线程数硬编码为4
    #         n_batch = 512     # 批处理大小硬编码为512
            
    #         logging.info(f"使用 LlamaCPP 模型: {model_path}")
            
    #         # 如果模型尚未加载，则加载模型
    #         if not hasattr(self, 'llamacpp_model') or self.llamacpp_model is None:
    #             logging.info(f"加载 LlamaCPP 模型中...")
    #             self.llamacpp_model = Llama(
    #                 model_path=model_path,
    #                 n_ctx=n_ctx,
    #                 n_threads=n_threads,
    #                 n_batch=n_batch
    #             )
    #             logging.info("LlamaCPP 模型加载完成")
            
    #         # 进行推理
    #         logging.debug(f"开始 LlamaCPP 推理，提示词前100字符: {prompt[:100]}...")
            
    #         # 是否使用流式推理
    #         if get_enable_stream():
    #             content, reasoning_content = self._process_llamacpp_stream(prompt)
    #         else:
    #             content, reasoning_content = self._process_llamacpp_non_stream(prompt)
                
    #         return {"content": content, "reasoning_content": reasoning_content}
            
    #     except ImportError:
    #         logging.error("未安装 llama-cpp-python 库，请执行: pip install llama-cpp-python")
    #         return {"content": "", "reasoning_content": ""}
    #     except Exception as e:
    #         logging.error(f"LlamaCPP 推理失败: {str(e)}")
    #         return {"content": "", "reasoning_content": ""}
        
    # def _process_llamacpp_stream(self, prompt):
    #     """处理 LlamaCPP 流式输出"""
    #     full_content = ""
    #     reasoning_content = ""
        
    #     try:
    #         # 硬编码最大生成token数
    #         max_tokens = 2048
            
    #         # 使用 LlamaCPP 的流式生成功能
    #         for output in self.llamacpp_model.create_completion(
    #             prompt,
    #             max_tokens=max_tokens,
    #             temperature=0.3,
    #             stream=True
    #         ):
    #             if "choices" in output and output["choices"]:
    #                 chunk = output["choices"][0].get("text", "")
    #                 full_content += chunk
    #                 logging.debug(f"接收流式数据: {len(chunk)} 字符")
        
    #         # 处理完整内容，移除 <think>...</think> 部分
    #         import re
    #         think_pattern = re.compile(r'<think>.*?</think>', re.DOTALL)
    #         full_content = think_pattern.sub('', full_content).strip()
            
    #         # 记录清理后的内容供调试
    #         logging.debug(f"处理后的内容前200字符: {full_content[:200]}...")
            
    #     except Exception as e:
    #         logging.error(f"流式处理失败: {str(e)}")
        
    #     return full_content, reasoning_content

    # def _process_llamacpp_non_stream(self, prompt):
    #     """处理 LlamaCPP 非流式输出"""
    #     content = ""
    #     reasoning_content = ""
        
    #     try:
    #         # 硬编码最大生成token数
    #         max_tokens = 2048
            
    #         # 执行推理
    #         output = self.llamacpp_model.create_completion(
    #             prompt,
    #             max_tokens=max_tokens,
    #             temperature=0.3,
    #             stream=False
    #         )
            
    #         if "choices" in output and output["choices"]:
    #             content = output["choices"][0].get("text", "")
            
    #         # 移除 <think>...</think> 部分
    #         import re
    #         think_pattern = re.compile(r'<think>.*?</think>', re.DOTALL)
    #         content = think_pattern.sub('', content).strip()
            
    #         # 记录清理后的内容供调试
    #         logging.debug(f"处理后的内容前200字符: {content[:200]}...")
            
    #     except Exception as e:
    #         logging.error(f"非流式处理失败: {str(e)}")
        
    #     return content, reasoning_content

#  """ This defines LLM related utility functions """

# import requests
# import logging
# from global_config import *

# # Suppress all warnings for cleaner output
# import warnings

# warnings.filterwarnings("ignore")


# class LLM:
#     def __init__(self, local_mode=False, ollama_url="http://localhost:11434"):
#         """Initialize LLM instance with the model ID and API headers."""
#         self.local_mode = local_mode
#         self.ollama_url = ollama_url
#         if not local_mode:
#             self.headers = {
#                 "Authorization": self.get_api_token(),
#                 "Content-Type": "application/json",
#             }
#         else:
#             self.headers = {
#                 "Content-Type": "application/json",
#             }
#         self.model_id = get_model_id()

#     def get_api_token(self) -> str:
#         """Return the API token required for authorization."""
#         return f"Bearer {llm_api_token}"

#     def get_model_id(self):
#         """Return the current LLM model ID."""
#         return self.model_id

#     def set_model_id(self, model_id: str):
#         """Set the model ID for the LLM."""
#         self.model_id = model_id

#     def inference(self, prompt):
#         if self.local_mode:
#             return self._ollama_inference(prompt)
#         else:
#             return self._cloud_inference(prompt)

#     def _cloud_inference(self, prompt):
#         payload = {
#             "model": self.model_id,
#             "messages": [
#                 {
#                     "role": "user",
#                     "content": prompt,
#                 }
#             ],
#             "temperature": 0.3,
#         }

#         response = requests.request(
#             "POST",
#             llm_url,
#             json=payload,
#             headers=self.headers,
#             verify=False,
#         )
#         # Successfully getting the inference response
#         if response.status_code == 200:
#             return response
#         else:
#             return None

#     def _ollama_inference(self, prompt):
#         """Use Ollama local service for inference"""
#         payload = {
#             "model": self.model_id,
#             "prompt": prompt,
#             "options": {
#                 "temperature": 0.3
#             }
#         }
        
#         try:
#             # 添加调试日志
#             logging.debug(f"Sending request to Ollama at {self.ollama_url}")
#             logging.debug(f"Payload: {payload}")
            
#             response = requests.post(
#                 f"{self.ollama_url}/api/generate",
#                 json=payload,
#                 headers=self.headers,
#                 verify=False,
#                 timeout=30  # 添加超时
#             )
            
#             # 添加响应日志
#             logging.debug(f"Ollama response status: {response.status_code}")
#             if response.status_code == 200:
#                 logging.debug(f"Response content: {response.text[:200]}...")  # 截取部分内容
#                 return response.json()
#             return None
#         except requests.exceptions.RequestException as e:
#             logging.error(f"Ollama request failed: {e}")
#             return None
