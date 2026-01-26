# Update: Audio Detection

## Change Made
The plugin now only sends audio to the backend when there's an actual audio signal being played, not just when recording mode is enabled.

## How It Works
- **Silence Detection**: The plugin checks each audio buffer for signal above -80dB
- **Smart Sending**: Audio is only transmitted when actual audio is present
- **Non-destructive**: Audio still passes through unchanged

## What This Means
- ✅ Clicking "Start Recording" enables send mode
- ✅ Audio is only sent when your DAW is playing
- ✅ No silent/empty chunks are transmitted
- ✅ More efficient bandwidth usage

## Testing
1. Load the updated plugin in GarageBand (it's been reinstalled)
2. Click "Start Recording"
3. Play audio through the track - you'll see backend activity
4. Stop playback - transmission stops automatically
5. Resume playback - transmission resumes

## Threshold
The silence threshold is set to `-80dB` (0.0001 amplitude). This ensures:
- Very quiet passages are still captured
- True silence is ignored
- Minimal overhead for detection

You can now test it in GarageBand!
