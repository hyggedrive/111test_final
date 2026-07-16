#!/bin/bash
set -euo pipefail

# 项目目录设置 (当前目录)
PROJECT_DIR=$(pwd)
THIRDPARTY_DIR="${PROJECT_DIR}/thirdparty"
LLAMACPP_DIR="${THIRDPARTY_DIR}/llama.cpp"
INCLUDE_DIR="${PROJECT_DIR}/include"
LIB_DIR="${PROJECT_DIR}/lib"

check_dependency() {
    if ! command -v "$1" &> /dev/null; then
        echo "错误: 未找到依赖工具 $1，请先安装并确保其在PATH中"
        exit 1
    fi
}
check_dependency make
# 添加Python3依赖检查
check_dependency python3

# ... 现有代码 ...

# 返回项目目录
cd "${PROJECT_DIR}"
echo "llamacpp库下载编译完成！头文件在${INCLUDE_DIR}，库文件在${LIB_DIR}"

# 创建model目录
echo "创建model目录..."
mkdir -p "${PROJECT_DIR}/model"

# 后台执行downmodel.py
if [ -f "download_models.py" ]; then
    echo "使用nohup后台执行downmodel.py..."
    nohup python3 download_models.py > download_models.log 2>&1 &
    echo "下载脚本已后台启动，日志输出到download_models.log"
else
    echo "警告: 未找到download_models.py文件，跳过执行"
fi
# 创建必要目录
mkdir -p "${THIRDPARTY_DIR}" "${INCLUDE_DIR}" "${LIB_DIR}"



check_dependency git
check_dependency cmake
check_dependency make

# 克隆llamacpp仓库
if [ ! -d "${LLAMACPP_DIR}" ]; then
    echo "克隆llamacpp仓库..."
    git clone https://github.com/ggerganov/llama.cpp.git "${LLAMACPP_DIR}"
else
    echo "llamacpp已存在，跳过克隆"
fi

# 编译llamacpp (静态库)
echo "开始编译llamacpp..."
cd "${LLAMACPP_DIR}"
mkdir -p build && cd build

# CMake配置 (默认生成Makefile，编译静态库)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DLLAMA_BUILD_TESTS=OFF
make -j$(nproc)

# # 复制头文件和库文件到项目目录
# echo "复制文件到项目目录..."
# cp "${LLAMACPP_DIR}/include/llama.h" "${INCLUDE_DIR}/"
# cp "${LLAMACPP_DIR}/build/libllama.a" "${LIB_DIR}/"

# # 返回项目目录
# cd "${PROJECT_DIR}"
# echo "llamacpp库下载编译完成！头文件在${INCLUDE_DIR}，库文件在${LIB_DIR}"

