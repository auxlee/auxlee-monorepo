from fastapi import FastAPI, File, UploadFile, Depends, HTTPException, status
from fastapi.security import HTTPBasic, HTTPBasicCredentials
from fastapi.responses import FileResponse
from typing import List, Optional
import secrets
import os
import uuid
import wave
import io
import logging
from pathlib import Path
from datetime import datetime

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

app = FastAPI(title="Auxlee Audio API", version="1.0.0")
security = HTTPBasic()

# Configuration
AUDIO_STORAGE_PATH = Path("./audio_storage")
AUDIO_STORAGE_PATH.mkdir(exist_ok=True)

# Simple authentication (replace with proper user management in production)
VALID_CREDENTIALS = {
    "admin": "password123"  # Change this in production!
}

# In-memory storage for track metadata
tracks_db = {}


def verify_credentials(credentials: HTTPBasicCredentials = Depends(security)):
    """Verify HTTP Basic Authentication credentials"""
    username = credentials.username
    password = credentials.password
    
    if username not in VALID_CREDENTIALS:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Basic"},
        )
    
    correct_password = VALID_CREDENTIALS[username]
    if not secrets.compare_digest(password.encode("utf8"), correct_password.encode("utf8")):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Incorrect username or password",
            headers={"WWW-Authenticate": "Basic"},
        )
    
    return username


class SessionManager:
    """Manages recording sessions and chunk assembly"""
    def __init__(self):
        self.sessions = {}
    
    def create_session(self, username: str) -> str:
        """Create a new recording session"""
        session_id = str(uuid.uuid4())
        session_path = AUDIO_STORAGE_PATH / f"session_{session_id}"
        session_path.mkdir(exist_ok=True)
        
        self.sessions[session_id] = {
            "username": username,
            "path": session_path,
            "chunks": [],
            "created_at": datetime.now(),
            "completed": False
        }
        
        logger.info(f"📝 Created new session {session_id[:8]}... for user '{username}'")
        return session_id
    
    def add_chunk(self, session_id: str, chunk_data: bytes) -> bool:
        """Add audio chunk to session"""
        if session_id not in self.sessions:
            logger.warning(f"⚠️  Chunk rejected: session {session_id[:8]}... not found")
            return False
        
        session = self.sessions[session_id]
        chunk_number = len(session["chunks"])
        chunk_path = session["path"] / f"chunk_{chunk_number:04d}.wav"
        
        with open(chunk_path, "wb") as f:
            f.write(chunk_data)
        
        session["chunks"].append(chunk_path)
        chunk_size_kb = len(chunk_data) / 1024
        logger.info(f"🎵 Chunk #{chunk_number} received: {chunk_size_kb:.2f} KB (session {session_id[:8]}...)")
        return True
    
    def finalize_session(self, session_id: str) -> Optional[str]:
        """Combine all chunks into final audio file"""
        if session_id not in self.sessions:
            return None
        
        session = self.sessions[session_id]
        if session["completed"]:
            return None
        
        # Create final audio file path
        track_id = str(uuid.uuid4())
        final_path = AUDIO_STORAGE_PATH / f"track_{track_id}.wav"
        
        # Combine all chunks
        if not session["chunks"]:
            return None
        
        try:
            # Open first chunk to get audio parameters
            with wave.open(str(session["chunks"][0]), 'rb') as first_wav:
                params = first_wav.getparams()
                
                # Create output file
                with wave.open(str(final_path), 'wb') as output_wav:
                    output_wav.setparams(params)
                    
                    # Write all chunks
                    for chunk_path in session["chunks"]:
                        with wave.open(str(chunk_path), 'rb') as chunk_wav:
                            output_wav.writeframes(chunk_wav.readframes(chunk_wav.getnframes()))
            
            # Store track metadata
            tracks_db[track_id] = {
                "id": track_id,
                "username": session["username"],
                "filename": final_path.name,
                "path": final_path,
                "created_at": datetime.now(),
                "session_id": session_id
            }
            
            session["completed"] = True
            total_chunks = len(session["chunks"])
            file_size_mb = final_path.stat().st_size / (1024 * 1024)
            logger.info(f"✅ Session {session_id[:8]}... finalized: {total_chunks} chunks → {file_size_mb:.2f} MB track")
            return track_id
            
        except Exception as e:
            logger.error(f"❌ Error finalizing session {session_id[:8]}...: {e}")
            return None


# Global session manager
session_manager = SessionManager()


@app.get("/")
async def root():
    """API root endpoint"""
    return {
        "message": "Auxlee Audio API",
        "version": "1.0.0",
        "status": "running"
    }


@app.post("/api/start-session")
async def start_session(username: str = Depends(verify_credentials)):
    """Start a new recording session"""
    logger.info(f"🎙️  User '{username}' starting new recording session")
    session_id = session_manager.create_session(username)
    return {
        "session_id": session_id,
        "message": "Recording session started"
    }


@app.post("/api/upload-chunk")
async def upload_chunk(
    file: UploadFile = File(...),
    session_id: Optional[str] = None,
    username: str = Depends(verify_credentials)
):
    """Receive audio chunk from plugin"""
    # Create session if not provided
    if session_id is None:
        logger.info(f"📝 Auto-creating session for user '{username}'")
        session_id = session_manager.create_session(username)
    
    # Read chunk data
    chunk_data = await file.read()
    logger.debug(f"📥 Receiving chunk from '{username}': {len(chunk_data)} bytes")
    
    # Add chunk to session
    success = session_manager.add_chunk(session_id, chunk_data)
    
    if not success:
        logger.error(f"❌ Failed to add chunk to session {session_id[:8]}...")
        raise HTTPException(status_code=404, detail="Session not found")
    
    return {
        "message": "Chunk received",
        "session_id": session_id,
        "chunk_size": len(chunk_data)
    }


@app.post("/api/finalize-session")
async def finalize_session(
    session_id: str,
    username: str = Depends(verify_credentials)
):
    """Finalize recording session and create complete track"""
    track_id = session_manager.finalize_session(session_id)
    
    if track_id is None:
        raise HTTPException(status_code=404, detail="Session not found or already completed")
    
    logger.info(f"✅ Session {session_id[:8]}... finalized, created track: {track_id[:8]}...")
    
    return {
        "message": "Session finalized",
        "track_id": track_id
    }


@app.get("/api/tracks")
async def list_tracks(username: str = Depends(verify_credentials)):
    """List all recorded tracks for authenticated user"""
    user_tracks = [
        {
            "id": track["id"],
            "filename": track["filename"],
            "created_at": track["created_at"].isoformat()
        }
        for track in tracks_db.values()
        if track["username"] == username
    ]
    
    return user_tracks


@app.get("/api/download/{track_id}")
async def download_track(
    track_id: str,
    username: str = Depends(verify_credentials)
):
    """Download a recorded track"""
    if track_id not in tracks_db:
        raise HTTPException(status_code=404, detail="Track not found")
    
    track = tracks_db[track_id]
    
    # Verify user owns this track
    if track["username"] != username:
        raise HTTPException(status_code=403, detail="Access denied")
    
    logger.info(f"⬇️  User '{username}' downloading track {track_id[:8]}... ({track['filename']})")
    return FileResponse(
        path=track["path"],
        media_type="audio/wav",
        filename=track["filename"]
    )


@app.delete("/api/tracks/{track_id}")
async def delete_track(
    track_id: str,
    username: str = Depends(verify_credentials)
):
    """Delete a recorded track"""
    if track_id not in tracks_db:
        raise HTTPException(status_code=404, detail="Track not found")
    
    track = tracks_db[track_id]
    
    # Verify user owns this track
    if track["username"] != username:
        raise HTTPException(status_code=403, detail="Access denied")
    
    # Delete file
    try:
        os.remove(track["path"])
        del tracks_db[track_id]
        return {"message": "Track deleted successfully"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error deleting track: {str(e)}")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
