#!/bin/sh
pip install pyinstaller
rm -rf build/
rm -rf dist/
rm -f bishengai.spec
TARGET="bishengai"
SOURCE="src/compiler_driver.py"
pyinstaller --onefile $SOURCE --name $TARGET
export CC=`pwd`/dist/$TARGET
export CXX=$CC
export LLM_DEVELOPMENT=0
# export COMPILER_CHOICE="clang++" # "clang" for c; "clang++"" for c++
export LLM_DEBUG=1
export AUTO_ACCEPT=1 # not export-interactive window; export-automatically change


# 1：流式 0：非流式
export ENABLE_STREAM=0

# openai
# export LLM_MODEL_TYPE=openai
# export LLM_API_TOKEN=xxxxxxxxxxxxxx
# # export LLM_MODEL=deepseek-ai/DeepSeek-R1-Distill-Qwen-14B
# export LLM_MODEL=Qwen/Qwen3-14B
# export LLM_URL=https://api.siliconflow.cn/v1/chat/completions


export LLM_MODEL_TYPE=openai
export LLM_MODEL=./model/Qwen3-14B-Q4_K_M.gguf
export LLM_URL=http://127.0.0.1:8080/v1/chat/completions