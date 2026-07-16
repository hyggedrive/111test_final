"""
This defines the global configurations for the compiler driver
"""

import logging
from enum import Enum
import os
from termcolor import colored

# Global Constants
LOG_FILE = "llm4compiler.log"

class ModelType(str, Enum):
    LOCAL_OLLAMA = "local_ollama"    # 本地Ollama部署
    OPENAI = "openai"                # OpenAI API
    LLAMA_CPP_CPU = "llama_cpp_cpu"  # llama.cpp CPU部署


# Function to get the api token for siliconflow
def get_llm_api_token() -> str:
    """Return the api token for LLM"""
    return os.getenv("LLM_API_TOKEN", "")
llm_api_token = get_llm_api_token()

def get_model_type() -> ModelType:
    """Return the LLM model type"""
    return ModelType(os.getenv("LLM_MODEL_TYPE", ModelType.OPENAI))

def get_enable_stream() -> bool:
    value = os.getenv("ENABLE_STREAM", "0")
    mapping = {
        "1": True,
        "0": False
    }
    return mapping.get(value, False)

def get_llm_url() -> str:
    """Return the LLM url"""
    return os.getenv("LLM_URL", "")


# Function to check if automatic acceptance of LLM code changes is enabled
def is_auto_accept_code_change() -> bool:
    """Return True if the user opts to automatically accept LLM suggested code changes."""
    return bool(os.getenv("AUTO_ACCEPT", False))


# Function to check if the compiler driver is running in development mode
def is_development_mode() -> bool:
    """Return True if running in development mode."""
    # return bool(os.getenv("LLM_DEVELOPMENT", True))  # Default to True if not set
    return False

# Return the LLM model ID, default to a preconfigured model
def get_model_id() -> str:
    """Return the LLM model ID, with a fallback if not set in environment."""
    return os.getenv("LLM_MODEL", "deepseek-ai/DeepSeek-R1-Distill-Llama-70B")


# Use a smaller model for simple tasks
def get_small_model_id() -> str:
    """Return the LLM small model ID, with a fallback if not set in environment."""
    return os.getenv("SMALL_LLM_MODEL", "deepseek-ai/DeepSeek-R1-Distill-Qwen-14B")


# Function to get the maximum retry times for LLM interactions
def get_llm_retry_times() -> int:
    """Return the number of retry attempts for LLM. Default is 5, max 10."""
    try:
        retry_times = int(os.getenv("LLM_RETRY_TIMES", 5))
        logging.info(f"LLM retry times: {retry_times}")
        return min(max(retry_times, 1), 10)  # Ensure retry times is between 1 and 10
    except ValueError:
        return 5


def try_small_llm_first():
    """Return true to try using a small LLM first for repairing"""
    return False


def single_source_file():
    """Return true to try using a small LLM first for repairing"""
    if os.getenv("SINGLE_SOURCE"):
        return True
    return False


# Set up the logging level based on environment variables
def set_logging_level() -> int:
    """Determine the logging level based on environment variables."""
    if os.getenv("LLM_DEBUG"):
        return logging.DEBUG
    elif os.getenv("LLM_SILENT"):
        return logging.ERROR
    else:
        return logging.INFO


# Custom logging formatter with color
class ColoredFormatter(logging.Formatter):
    COLORS = {
        "DEBUG": "cyan",
        "INFO": "green",
        "WARNING": "yellow",
        "ERROR": "red",
        "CRITICAL": "magenta",
    }

    def format(self, record):
        levelname = record.levelname
        message = super().format(record)
        return colored(message, self.COLORS.get(levelname, "white"))


# Set up logging with colored output
def configure_logging():
    """Configure logging with colored output and file logging."""
    formatter = ColoredFormatter(
        "%(asctime)s - %(levelname)s - %(message)s", datefmt="%Y-%m-%d %H:%M:%S"
    )

    logger = logging.getLogger()
    logger.setLevel(set_logging_level())

    # Console handler for colored output
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)

    # File handler for logging to file
    file_handler = logging.FileHandler(LOG_FILE, mode="w", encoding="utf-8")
    file_handler.setLevel(logging.DEBUG)  # Log everything to the file
    logger.addHandler(file_handler)


# Initialize logging
configure_logging()

def get_llamacpp_model_path():
    """获取 LlamaCPP 模型路径"""
    return os.environ.get("LLAMACPP_MODEL_PATH", "/path/to/default/model.gguf")
