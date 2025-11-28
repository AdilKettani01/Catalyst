#!/bin/bash
# Convert PPM frame sequence to MP4 video
#
# Usage:
#   ./frames_to_mp4.sh [input_dir] [output.mp4] [fps]
#
# Examples:
#   ./frames_to_mp4.sh                              # Use defaults
#   ./frames_to_mp4.sh media/frames output.mp4 60  # Custom settings
#
# Prerequisites:
#   - FFmpeg must be installed: sudo apt install ffmpeg
#   - Frame files must be named: frame_0001.ppm, frame_0002.ppm, etc.

set -e

INPUT_DIR="${1:-media/frames}"
OUTPUT="${2:-output.mp4}"
FPS="${3:-60}"

# Check if input directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Input directory '$INPUT_DIR' does not exist"
    exit 1
fi

# Check if there are PPM files
PPM_COUNT=$(ls -1 "$INPUT_DIR"/frame_*.ppm 2>/dev/null | wc -l)
if [ "$PPM_COUNT" -eq 0 ]; then
    echo "Error: No frame_*.ppm files found in '$INPUT_DIR'"
    exit 1
fi

echo "Converting $PPM_COUNT frames from '$INPUT_DIR' to '$OUTPUT' at ${FPS}fps..."

ffmpeg -y -framerate "$FPS" -i "$INPUT_DIR/frame_%04d.ppm" \
       -c:v libx264 -pix_fmt yuv420p -crf 18 \
       -movflags +faststart \
       "$OUTPUT"

echo ""
echo "Created: $OUTPUT"
echo "Frames:  $PPM_COUNT"
echo "FPS:     $FPS"

# Show file size
if [ -f "$OUTPUT" ]; then
    SIZE=$(du -h "$OUTPUT" | cut -f1)
    echo "Size:    $SIZE"
fi
