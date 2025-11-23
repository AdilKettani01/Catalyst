#!/bin/bash

# =============================================================================
# Manim C++ Complete Validation Script
# =============================================================================
#
# This script performs comprehensive validation of the entire Manim C++
# conversion including:
# - Build verification
# - Unit tests
# - Integration tests
# - Performance benchmarks
# - Python compatibility
# - Visual regression
# - GPU validation
# - Memory leak detection
#
# Usage: ./validate_all.sh [--skip-build] [--quick] [--verbose]
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
VALIDATION_DIR="validation_results"
SKIP_BUILD=false
QUICK_MODE=false
VERBOSE=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --quick)
            QUICK_MODE=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --skip-build    Skip building the project"
            echo "  --quick         Run quick validation (skip long tests)"
            echo "  --verbose       Show detailed output"
            echo "  --help          Show this help message"
            exit 0
            ;;
    esac
done

# Logging functions
log_step() {
    echo -e "${BLUE}[$(date +'%T')]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Create validation results directory
mkdir -p "$VALIDATION_DIR"

# Start validation
echo ""
echo "=========================================="
echo "MANIM C++ COMPLETE VALIDATION"
echo "=========================================="
echo ""
echo "Timestamp: $(date)"
echo "Build directory: $BUILD_DIR"
echo "Results directory: $VALIDATION_DIR"
echo ""

# =============================================================================
# 1. BUILD PROJECT
# =============================================================================

if [ "$SKIP_BUILD" = false ]; then
    log_step "[1/10] Building project..."

    # Clean build directory if it exists
    if [ -d "$BUILD_DIR" ]; then
        log_warning "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi

    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure with CMake
    if [ "$VERBOSE" = true ]; then
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DMANIM_BUILD_TESTS=ON \
            -DMANIM_BUILD_BENCHMARKS=ON \
            -DMANIM_BUILD_PYTHON_BINDINGS=ON \
            -DMANIM_ENABLE_VULKAN=ON \
            -DMANIM_ENABLE_RAYTRACING=ON
    else
        cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DMANIM_BUILD_TESTS=ON \
            -DMANIM_BUILD_BENCHMARKS=ON \
            -DMANIM_BUILD_PYTHON_BINDINGS=ON \
            -DMANIM_ENABLE_VULKAN=ON \
            -DMANIM_ENABLE_RAYTRACING=ON \
            > "$VALIDATION_DIR/cmake_output.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "CMake configuration successful"
    else
        log_error "CMake configuration failed"
        cat "$VALIDATION_DIR/cmake_output.log"
        exit 1
    fi

    # Build
    log_step "Compiling C++ code..."
    if [ "$VERBOSE" = true ]; then
        cmake --build . --config Release -j$(nproc)
    else
        cmake --build . --config Release -j$(nproc) \
            > "$VALIDATION_DIR/build_output.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "Build successful"
    else
        log_error "Build failed"
        cat "$VALIDATION_DIR/build_output.log"
        exit 1
    fi

    cd ..
else
    log_step "[1/10] Skipping build (--skip-build specified)"
fi

# =============================================================================
# 2. UNIT TESTS
# =============================================================================

log_step "[2/10] Running unit tests..."

if [ -f "$BUILD_DIR/manim_tests" ]; then
    if [ "$VERBOSE" = true ]; then
        $BUILD_DIR/manim_tests --gtest_output=xml:$VALIDATION_DIR/unit_tests.xml
    else
        $BUILD_DIR/manim_tests --gtest_output=xml:$VALIDATION_DIR/unit_tests.xml \
            > "$VALIDATION_DIR/unit_tests.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "Unit tests passed"
    else
        log_error "Unit tests failed"
        cat "$VALIDATION_DIR/unit_tests.log"
        exit 1
    fi
else
    log_warning "Unit tests binary not found"
fi

# =============================================================================
# 3. SYSTEM VERIFICATION
# =============================================================================

log_step "[3/10] Running system verification..."

if [ -f "$BUILD_DIR/system_verification" ]; then
    if [ "$VERBOSE" = true ]; then
        $BUILD_DIR/system_verification
    else
        $BUILD_DIR/system_verification > "$VALIDATION_DIR/system_verification.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "System verification passed"
    else
        log_warning "Some system components may not be fully functional"
    fi

    # Copy report
    if [ -f "system_verification_report.md" ]; then
        cp system_verification_report.md "$VALIDATION_DIR/"
    fi
else
    log_warning "System verification binary not found"
fi

# =============================================================================
# 4. GPU AND 3D INTEGRATION TESTS
# =============================================================================

log_step "[4/10] Running GPU and 3D integration tests..."

if [ -f "$BUILD_DIR/gpu_3d_integration_tests" ]; then
    if [ "$VERBOSE" = true ]; then
        $BUILD_DIR/gpu_3d_integration_tests
    else
        $BUILD_DIR/gpu_3d_integration_tests > "$VALIDATION_DIR/gpu_3d_tests.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "GPU/3D integration tests passed"
    else
        log_warning "Some GPU/3D tests failed (may not have GPU available)"
    fi
else
    log_warning "GPU/3D integration tests binary not found"
fi

# =============================================================================
# 5. PERFORMANCE BENCHMARKS
# =============================================================================

if [ "$QUICK_MODE" = false ]; then
    log_step "[5/10] Running performance benchmarks..."

    if [ -f "$BUILD_DIR/manim_benchmarks" ]; then
        if [ "$VERBOSE" = true ]; then
            $BUILD_DIR/manim_benchmarks \
                --benchmark_out=$VALIDATION_DIR/benchmarks.json \
                --benchmark_out_format=json
        else
            $BUILD_DIR/manim_benchmarks \
                --benchmark_out=$VALIDATION_DIR/benchmarks.json \
                --benchmark_out_format=json \
                > "$VALIDATION_DIR/benchmarks.log" 2>&1
        fi

        if [ $? -eq 0 ]; then
            log_success "Performance benchmarks completed"
        else
            log_warning "Some benchmarks failed"
        fi

        # Copy HTML report if generated
        if [ -f "performance_report.html" ]; then
            cp performance_report.html "$VALIDATION_DIR/"
        fi
    else
        log_warning "Benchmark binary not found"
    fi
else
    log_step "[5/10] Skipping benchmarks (--quick mode)"
fi

# =============================================================================
# 6. PYTHON COMPATIBILITY TESTS
# =============================================================================

log_step "[6/10] Running Python compatibility tests..."

# Check if Python bindings were built
if [ -f "$BUILD_DIR/python/manim_cpp/manim_cpp.so" ] || [ -f "$BUILD_DIR/python/manim_cpp/manim_cpp.pyd" ]; then
    # Add Python module to path
    export PYTHONPATH="$BUILD_DIR/python:$PYTHONPATH"

    if [ -f "manim-cpp/tests/validation/test_python_compatibility.py" ]; then
        if [ "$VERBOSE" = true ]; then
            python3 manim-cpp/tests/validation/test_python_compatibility.py
        else
            python3 manim-cpp/tests/validation/test_python_compatibility.py \
                > "$VALIDATION_DIR/python_tests.log" 2>&1
        fi

        if [ $? -eq 0 ]; then
            log_success "Python compatibility tests passed"
        else
            log_warning "Some Python tests failed"
        fi
    else
        log_warning "Python test script not found"
    fi
else
    log_warning "Python bindings not found (build with -DMANIM_BUILD_PYTHON_BINDINGS=ON)"
fi

# =============================================================================
# 7. VISUAL REGRESSION TESTS
# =============================================================================

if [ "$QUICK_MODE" = false ]; then
    log_step "[7/10] Running visual regression tests..."

    if [ -f "$BUILD_DIR/visual_regression_tests" ]; then
        if [ "$VERBOSE" = true ]; then
            $BUILD_DIR/visual_regression_tests
        else
            $BUILD_DIR/visual_regression_tests > "$VALIDATION_DIR/visual_tests.log" 2>&1
        fi

        if [ $? -eq 0 ]; then
            log_success "Visual regression tests passed"
        else
            log_warning "Some visual tests failed"
        fi
    else
        log_warning "Visual regression tests not found"
    fi
else
    log_step "[7/10] Skipping visual regression (--quick mode)"
fi

# =============================================================================
# 8. GPU-SPECIFIC VALIDATION
# =============================================================================

log_step "[8/10] Running GPU-specific validation..."

# Check if GPU is available
if command -v nvidia-smi &> /dev/null; then
    log_success "NVIDIA GPU detected"
    nvidia-smi > "$VALIDATION_DIR/gpu_info.txt" 2>&1
elif command -v rocm-smi &> /dev/null; then
    log_success "AMD GPU detected"
    rocm-smi > "$VALIDATION_DIR/gpu_info.txt" 2>&1
else
    log_warning "No GPU detected (CPU fallback will be used)"
fi

# Run GPU tests from unit tests
if [ -f "$BUILD_DIR/manim_tests" ]; then
    if [ "$VERBOSE" = true ]; then
        $BUILD_DIR/manim_tests --gtest_filter=GPU* --gtest_filter=GPUOptimization*
    else
        $BUILD_DIR/manim_tests --gtest_filter=GPU* --gtest_filter=GPUOptimization* \
            > "$VALIDATION_DIR/gpu_validation.log" 2>&1
    fi

    if [ $? -eq 0 ]; then
        log_success "GPU validation passed"
    else
        log_warning "Some GPU tests failed (check if GPU is available)"
    fi
fi

# =============================================================================
# 9. MEMORY LEAK DETECTION
# =============================================================================

if [ "$QUICK_MODE" = false ]; then
    log_step "[9/10] Checking for memory leaks..."

    if command -v valgrind &> /dev/null; then
        if [ -f "$BUILD_DIR/manim_tests" ]; then
            log_step "Running valgrind memory check (this may take a while)..."

            valgrind \
                --leak-check=full \
                --show-leak-kinds=all \
                --track-origins=yes \
                --log-file="$VALIDATION_DIR/valgrind.log" \
                $BUILD_DIR/manim_tests \
                --gtest_filter=*Memory* \
                > /dev/null 2>&1

            # Check for memory leaks
            if grep -q "definitely lost: 0 bytes" "$VALIDATION_DIR/valgrind.log"; then
                log_success "No memory leaks detected"
            else
                log_warning "Possible memory leaks detected (see valgrind.log)"
            fi
        fi
    else
        log_warning "Valgrind not found (skipping memory leak detection)"
    fi
else
    log_step "[9/10] Skipping memory leak detection (--quick mode)"
fi

# =============================================================================
# 10. GENERATE FINAL REPORT
# =============================================================================

log_step "[10/10] Generating validation report..."

# Create comprehensive report
REPORT_FILE="$VALIDATION_DIR/validation_report.md"

cat > "$REPORT_FILE" << EOF
# Manim C++ Validation Report

**Generated:** $(date)

## Summary

This report contains the results of comprehensive validation of the Manim C++ conversion.

## Build Status

- **Build Type:** Release
- **CMake Version:** $(cmake --version | head -n1)
- **Compiler:** $(c++ --version | head -n1)

## Test Results

### Unit Tests
EOF

if [ -f "$VALIDATION_DIR/unit_tests.log" ]; then
    echo "✓ Unit tests completed" >> "$REPORT_FILE"
    echo "\`\`\`" >> "$REPORT_FILE"
    tail -20 "$VALIDATION_DIR/unit_tests.log" >> "$REPORT_FILE"
    echo "\`\`\`" >> "$REPORT_FILE"
else
    echo "✗ Unit tests not run" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

### System Verification
EOF

if [ -f "$VALIDATION_DIR/system_verification_report.md" ]; then
    cat "$VALIDATION_DIR/system_verification_report.md" >> "$REPORT_FILE"
else
    echo "System verification report not generated" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

### Performance Benchmarks
EOF

if [ -f "$VALIDATION_DIR/benchmarks.json" ]; then
    echo "Benchmark results available in benchmarks.json" >> "$REPORT_FILE"
else
    echo "Benchmarks not run" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

### GPU Information
EOF

if [ -f "$VALIDATION_DIR/gpu_info.txt" ]; then
    echo "\`\`\`" >> "$REPORT_FILE"
    cat "$VALIDATION_DIR/gpu_info.txt" >> "$REPORT_FILE"
    echo "\`\`\`" >> "$REPORT_FILE"
else
    echo "No GPU information available" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

### Memory Leak Detection
EOF

if [ -f "$VALIDATION_DIR/valgrind.log" ]; then
    echo "\`\`\`" >> "$REPORT_FILE"
    grep "LEAK SUMMARY" -A 10 "$VALIDATION_DIR/valgrind.log" >> "$REPORT_FILE"
    echo "\`\`\`" >> "$REPORT_FILE"
else
    echo "Memory leak detection not run" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

## Files Generated

- System Verification Report: system_verification_report.md
- Performance Report: performance_report.html
- Unit Test Results: unit_tests.xml
- Benchmark Results: benchmarks.json

## Conclusion

All validation steps completed. See individual log files for details.
EOF

log_success "Validation report generated: $REPORT_FILE"

# =============================================================================
# FINAL STATUS
# =============================================================================

echo ""
echo "=========================================="
echo "VALIDATION COMPLETE"
echo "=========================================="
echo ""
echo "Results saved to: $VALIDATION_DIR/"
echo ""
log_success "All validation steps completed!"
echo ""
echo "Key files:"
echo "  - Validation Report:  $VALIDATION_DIR/validation_report.md"
echo "  - System Verification: $VALIDATION_DIR/system_verification_report.md"
echo "  - Performance Report:  $VALIDATION_DIR/performance_report.html"
echo ""
echo "View the full report:"
echo "  cat $VALIDATION_DIR/validation_report.md"
echo ""
echo "View performance results:"
echo "  open $VALIDATION_DIR/performance_report.html"
echo ""
echo "=========================================="
