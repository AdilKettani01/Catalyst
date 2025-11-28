import manim_cpp as m
print("GPU available:", m.check_gpu_available())
try:
    info = m.get_gpu_info()
    print("GPU info count:", len(info))
    for gpu in info:
        print(" -", gpu.name, "| mem MB:", getattr(gpu, "total_memory_mb", "n/a"))
except Exception as e:
    print("Failed to get GPU info:", e)
