"""
Manim C++ - GPU-accelerated mathematical animation engine
Python bindings for the C++ implementation

This module provides Python access to the high-performance C++ Manim implementation
with full GPU acceleration support.
"""

# Import the compiled C++ module
try:
    from . import manim_cpp
except ImportError as e:
    raise ImportError(
        "Failed to import manim_cpp module. "
        "Make sure the C++ extension has been compiled. "
        f"Error: {e}"
    )

# ==================== Export all core types ====================

# Core types
Vec3 = manim_cpp.Vec3
Vec4 = manim_cpp.Vec4
Mat4 = manim_cpp.Mat4

# Constants
ORIGIN = manim_cpp.ORIGIN
UP = manim_cpp.UP
DOWN = manim_cpp.DOWN
LEFT = manim_cpp.LEFT
RIGHT = manim_cpp.RIGHT
IN = manim_cpp.IN
OUT = manim_cpp.OUT
PI = manim_cpp.PI
TAU = manim_cpp.TAU
DEGREES = manim_cpp.DEGREES

# Vector3 alias
Vector3 = manim_cpp.Vector3

# Color constants
RED = manim_cpp.RED
GREEN = manim_cpp.GREEN
BLUE = manim_cpp.BLUE
YELLOW = manim_cpp.YELLOW
PURPLE = manim_cpp.PURPLE
ORANGE = manim_cpp.ORANGE
WHITE = manim_cpp.WHITE
BLACK = manim_cpp.BLACK
GREY = manim_cpp.GREY
GRAY = manim_cpp.GRAY
PINK = manim_cpp.PINK
TEAL = manim_cpp.TEAL

# Color class
Color = manim_cpp.Color

# ==================== Export mobject classes ====================

# Base mobjects
Mobject = manim_cpp.Mobject
VMobject = manim_cpp.VMobject

# Geometry mobjects
Circle = manim_cpp.Circle
Dot = manim_cpp.Dot
Ellipse = manim_cpp.Ellipse
Arc = manim_cpp.Arc
Annulus = manim_cpp.Annulus
Rectangle = manim_cpp.Rectangle
Square = manim_cpp.Square
Polygon = manim_cpp.Polygon

# Point cloud mobjects
PointCloudMobject = manim_cpp.PointCloudMobject
PMobject = manim_cpp.PMobject  # Alias
BillboardMode = manim_cpp.BillboardMode

# Constants
DEFAULT_DOT_RADIUS = manim_cpp.DEFAULT_DOT_RADIUS

# Text mobjects
Text = manim_cpp.Text
Tex = manim_cpp.Tex
MathTex = manim_cpp.MathTex
TextWeight = manim_cpp.TextWeight
TextAlignment = manim_cpp.TextAlignment

# 3D mobjects
GPUMesh = manim_cpp.GPUMesh
Mesh = manim_cpp.Mesh  # Alias
Sphere = manim_cpp.Sphere
Surface = manim_cpp.Surface
Volume = manim_cpp.Volume

# ==================== Export animation classes ====================

# Base animation
Animation = manim_cpp.Animation

# Creation animations
ShowCreation = manim_cpp.ShowCreation
Uncreate = manim_cpp.Uncreate
DrawBorderThenFill = manim_cpp.DrawBorderThenFill
Write = manim_cpp.Write
Unwrite = manim_cpp.Unwrite
ShowPartial = manim_cpp.ShowPartial
ShowIncreasingSubsets = manim_cpp.ShowIncreasingSubsets
ShowSubmobjectsOneByOne = manim_cpp.ShowSubmobjectsOneByOne

# Fading animations
FadeIn = manim_cpp.FadeIn
FadeOut = manim_cpp.FadeOut
FadeInFromPoint = manim_cpp.FadeInFromPoint
FadeOutToPoint = manim_cpp.FadeOutToPoint
FadeInFromLarge = manim_cpp.FadeInFromLarge
FadeTransform = manim_cpp.FadeTransform

# Transform animations
Transform = manim_cpp.Transform
ReplacementTransform = manim_cpp.ReplacementTransform
TransformFromCopy = manim_cpp.TransformFromCopy
ClockwiseTransform = manim_cpp.ClockwiseTransform
CounterclockwiseTransform = manim_cpp.CounterclockwiseTransform
MoveToTarget = manim_cpp.MoveToTarget
ApplyFunction = manim_cpp.ApplyFunction
ApplyMatrix = manim_cpp.ApplyMatrix
ApplyComplexFunction = manim_cpp.ApplyComplexFunction

# Animation composition
AnimationGroup = manim_cpp.AnimationGroup
Succession = manim_cpp.Succession
LaggedStart = manim_cpp.LaggedStart

# Movement animations
MoveAlongPath = manim_cpp.MoveAlongPath
MoveTo = manim_cpp.MoveTo
Shift = manim_cpp.Shift
Homotopy = manim_cpp.Homotopy
ComplexHomotopy = manim_cpp.ComplexHomotopy
ApplyPointwiseFunction = manim_cpp.ApplyPointwiseFunction
ApplyComplexPointwiseFunction = manim_cpp.ApplyComplexPointwiseFunction

# ==================== Export scene classes ====================

Scene = manim_cpp.Scene
ThreeDScene = manim_cpp.ThreeDScene
GPU3DScene = manim_cpp.GPU3DScene
MovingCameraScene = manim_cpp.MovingCameraScene
VectorSpaceScene = manim_cpp.VectorSpaceScene
ZoomedScene = manim_cpp.ZoomedScene

# Light and lighting types
Light = manim_cpp.Light
LightType = manim_cpp.LightType

# ==================== Export utility modules ====================

# Bezier utilities
bezier = manim_cpp.bezier

# Color utilities
color = manim_cpp.color

# Rate functions
rate_functions = manim_cpp.rate_functions

# Common rate functions (for convenience)
linear = rate_functions.linear
smooth = rate_functions.smooth
there_and_back = rate_functions.there_and_back
ease_in_sine = rate_functions.ease_in_sine
ease_out_sine = rate_functions.ease_out_sine
ease_in_out_sine = rate_functions.ease_in_out_sine

# ==================== Export configuration ====================

GPUBackend = manim_cpp.GPUBackend
QualityPreset = manim_cpp.QualityPreset
RenderConfig = manim_cpp.RenderConfig
GPUDetector = manim_cpp.GPUDetector

# Global config accessor
get_config = manim_cpp.get_config

# Convenience functions
check_gpu_available = manim_cpp.check_gpu_available
get_gpu_info = manim_cpp.get_gpu_info

# ==================== Culling/Optimization API ====================

# Culling submodule
culling = manim_cpp.culling

# ==================== GPU Utilities ====================

# GPU types
GPUDeviceInfo = manim_cpp.GPUDeviceInfo
PBRMaterial = manim_cpp.PBRMaterial
GPUParticleSystem = manim_cpp.GPUParticleSystem
ParticleEmitterConfig = manim_cpp.ParticleEmitterConfig

# GPU utility functions
transform_points_gpu = manim_cpp.transform_points_gpu
rotation_matrix = manim_cpp.rotation_matrix

# ==================== Version and metadata ====================

__version__ = manim_cpp.__version__
__gpu_enabled__ = manim_cpp.__gpu_enabled__

# ==================== Convenience aliases (for backward compatibility) ====================

# Common animation aliases
Create = ShowCreation  # Alias for compatibility

# Direction vectors (for convenience)
UR = Vec3(1, 1, 0)
UL = Vec3(-1, 1, 0)
DR = Vec3(1, -1, 0)
DL = Vec3(-1, -1, 0)

# ==================== All exports ====================

__all__ = [
    # Core types
    'Vec3', 'Vec4', 'Mat4', 'Vector3', 'Color',

    # Constants
    'ORIGIN', 'UP', 'DOWN', 'LEFT', 'RIGHT', 'IN', 'OUT',
    'UR', 'UL', 'DR', 'DL',
    'PI', 'TAU', 'DEGREES',
    'DEFAULT_DOT_RADIUS',

    # Color constants
    'RED', 'GREEN', 'BLUE', 'YELLOW', 'PURPLE', 'ORANGE',
    'WHITE', 'BLACK', 'GREY', 'GRAY', 'PINK', 'TEAL',

    # Mobjects
    'Mobject', 'VMobject',
    'Circle', 'Dot', 'Ellipse', 'Arc', 'Annulus',
    'Rectangle', 'Square', 'Polygon',
    'PointCloudMobject', 'PMobject', 'BillboardMode',
    'Text', 'Tex', 'MathTex', 'TextWeight', 'TextAlignment',
    'GPUMesh', 'Mesh', 'Sphere', 'Surface', 'Volume',

    # Animations
    'Animation',
    'ShowCreation', 'Create', 'Uncreate', 'DrawBorderThenFill',
    'Write', 'Unwrite', 'ShowPartial', 'ShowIncreasingSubsets',
    'ShowSubmobjectsOneByOne',
    'FadeIn', 'FadeOut', 'FadeInFromPoint', 'FadeOutToPoint',
    'FadeInFromLarge', 'FadeTransform',
    'Transform', 'ReplacementTransform', 'TransformFromCopy',
    'ClockwiseTransform', 'CounterclockwiseTransform', 'MoveToTarget',
    'ApplyFunction', 'ApplyMatrix', 'ApplyComplexFunction',
    'AnimationGroup', 'Succession', 'LaggedStart',
    'MoveAlongPath', 'MoveTo', 'Shift',
    'Homotopy', 'ComplexHomotopy',
    'ApplyPointwiseFunction', 'ApplyComplexPointwiseFunction',

    # Scenes
    'Scene', 'ThreeDScene', 'GPU3DScene', 'MovingCameraScene',
    'VectorSpaceScene', 'ZoomedScene',
    'Light', 'LightType', 'PBRMaterial',

    # Utilities
    'bezier', 'color', 'rate_functions',
    'linear', 'smooth', 'there_and_back',
    'ease_in_sine', 'ease_out_sine', 'ease_in_out_sine',

    # Configuration
    'GPUBackend', 'QualityPreset', 'RenderConfig', 'GPUDetector',
    'GPUDeviceInfo',
    'get_config', 'check_gpu_available', 'get_gpu_info',

    # Culling/Optimization
    'culling',

    # GPU utilities
    'GPUParticleSystem', 'ParticleEmitterConfig',
    'transform_points_gpu', 'rotation_matrix',

    # Metadata
    '__version__', '__gpu_enabled__',
]


# ==================== Helper functions ====================

def check_system():
    """
    Print comprehensive system information and GPU capabilities.

    This function creates a GPUDetector, runs detection, and prints
    a full system report including GPU info, capabilities, and recommendations.
    """
    detector = GPUDetector()
    detector.detect_gpus()
    detector.check_vulkan_support()
    detector.print_system_report()

    print("\nRecommended settings:")
    config = detector.recommend_settings()
    print(f"  Quality: {config.quality}")
    print(f"  Resolution: {config.width}x{config.height}")
    print(f"  Backend: {config.backend}")
    print(f"  Ray Tracing: {config.enable_ray_tracing}")
    print(f"  Global Illumination: {config.enable_gi}")


def get_recommended_config():
    """
    Get recommended render configuration based on detected hardware.

    Returns:
        RenderConfig: Optimized configuration for current system
    """
    detector = GPUDetector()
    detector.detect_gpus()
    return detector.recommend_settings()


# ==================== Quick start guide ====================

def quick_start():
    """Print quick start guide for Manim C++"""
    print("""
╔════════════════════════════════════════════════════════════════╗
║                    Manim C++ Quick Start                       ║
╚════════════════════════════════════════════════════════════════╝

Manim C++ is a GPU-accelerated mathematical animation engine.

Basic Example:
--------------
from manim_cpp import *

class MyScene(Scene):
    def construct(self):
        circle = Circle()
        circle.set_color(Vec4(1, 0, 0, 1))  # Red
        self.add(circle)
        self.play(ShowCreation(circle))
        self.wait()

GPU-Accelerated 3D Scene:
-------------------------
class My3DScene(GPU3DScene):
    def construct(self):
        # Setup GPU features
        self.setup_deferred_pipeline()
        self.setup_ray_tracing()

        # Add light
        light = Light()
        light.position = Vec3(5, 5, 5)
        self.add_light(light)

        # Render with PBR
        mesh = Mesh()
        self.add(mesh)
        self.render_with_pbr()

Configuration:
--------------
# Check GPU availability
if check_gpu_available():
    print("GPU acceleration available!")

# Get recommended settings
config = get_recommended_config()

# Check system capabilities
check_system()

For more information, visit the documentation.
    """)


if __name__ == "__main__":
    # If module is run directly, show quick start guide
    quick_start()
