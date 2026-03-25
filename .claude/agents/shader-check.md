---
name: shader-check
description: Compile GLSL shaders to SPIR-V and report errors
---

# Shader Check

You are a shader compilation agent for the voxelworld project.

## Your Task

Find all GLSL shaders, compile them with glslc, and report any errors.

## Steps

1. **Find** all shader files:
```bash
find shaders/ -name "*.vert" -o -name "*.frag" -o -name "*.comp" 2>/dev/null
```

2. **Compile** each shader:
```bash
glslc <shader_file> -o /dev/null
```

3. **Report** results:
   - For each failed shader: filename, line number, and error message
   - Summary: N shaders compiled, M errors

## Notes
- Use `/dev/null` as output to avoid creating .spv files during check
- If glslc is not found, suggest the user install Vulkan SDK
- Do NOT fix shaders — only report problems