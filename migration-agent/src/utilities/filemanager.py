import logging
import shutil
import os


class FileManager:
    @staticmethod
    def backup_file(source, dest):
        """Backup a file to the destination."""
        try:
            logging.debug(f"Backing up {source} to {dest}")
            shutil.copy(source, dest)
            return True
        except Exception as e:
            logging.error(f"Failed to backup {source} to {dest}: {e}")
            return False

    @staticmethod
    def restore_files(source_paths, backup_path):
        """Restore files from the backup."""
        try:
            for file_name, path in source_paths.items():
                backup_file_path = os.path.join(backup_path, file_name)
                shutil.copy(backup_file_path, path)
                logging.debug(f"Restored {backup_file_path} to {path}")
            return True
        except Exception as e:
            logging.error(f"Failed to restore files: {e}")
            return False

    @staticmethod
    def create_folders_for_path(file_path):
        """Create necessary folders for a given file path."""
        dir_path = os.path.dirname(file_path)
        new_folders = []
        if dir_path and not os.path.exists(dir_path):
            logging.debug(f"Creating missing directories: {dir_path}")
            os.makedirs(dir_path)
            new_folders.append(dir_path)
        return new_folders
