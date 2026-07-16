from modelscope.hub.file_download import model_file_download
import os

# 目标保存目录
save_base_dir = "./model"
os.makedirs(save_base_dir, exist_ok=True)

# 定义需要下载的模型仓库和对应文件
model_files = [

    {
        "repo_id": "unsloth/Qwen3-14B-GGUF",
        "file_name": "Qwen3-14B-Q4_K_M.gguf"  # 请替换为实际存在的文件名
    }

]

# 批量下载指定文件
for item in model_files:
    repo_id = item["repo_id"]
    file_name = item["file_name"]
    save_path = os.path.join(save_base_dir, file_name)
    
    print(f"开始下载: {repo_id}/{file_name}")
    try:
        # 下载文件并指定保存路径
        downloaded_path = model_file_download(
            model_id=repo_id,
            file_path=file_name,
            local_dir=save_base_dir,
        )
        print(f"成功下载到: {downloaded_path}\n")
    except Exception as e:
        print(f"下载失败: {str(e)}\n")
