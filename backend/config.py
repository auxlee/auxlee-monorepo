"""
Configuration settings for the Auxlee Audio API
"""
from pydantic_settings import BaseSettings
from typing import Dict


class Settings(BaseSettings):
    """Application settings"""
    
    # API Settings
    api_title: str = "Auxlee Audio API"
    api_version: str = "1.0.0"
    api_host: str = "0.0.0.0"
    api_port: int = 8000
    
    # Storage Settings
    audio_storage_path: str = "./audio_storage"
    max_chunk_size_mb: int = 10
    
    # Authentication
    # In production, use environment variables and secure password hashing
    valid_users: Dict[str, str] = {
        "admin": "password123"
    }
    
    # Session Settings
    session_timeout_minutes: int = 60
    auto_finalize_on_timeout: bool = True
    
    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"


settings = Settings()
