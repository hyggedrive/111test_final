#!/bin/sh
# 修改模型路径为当前目录下的model目录
MODEL_PATH="./model/Qwen3-14B-Q4_K_M.gguf"
# 调整前缀提取规则以匹配新路径
MODEL_VAR="${MODEL_PATH#./model/}"
MODEL_VAR="${MODEL_VAR%.gguf}"
LOG_FILE="${MODEL_VAR}_$(date +%m%d%H%M%S).log"

nohup ./thirdparty/llama.cpp/build/bin/llama-server \
  -m "$MODEL_PATH" \
  > "$LOG_FILE" 2>&1 &