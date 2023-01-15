#include "GlShaderModule.h"

namespace Vixen::Engine {
    GlShaderModule::GlShaderModule(Stage stage, const std::string &source, const std::string &entry) : Engine::ShaderModule(stage, source, entry), module(0) {
        spirv_cross::CompilerGLSL compiler(binary);
        spirv_cross::CompilerGLSL::Options options {
            .version = 450,
            .es = false,
            .force_temporary = false,
            .force_recompile_max_debug_iterations = 3,
            .vulkan_semantics = false,
            .separate_shader_objects = true,
            .flatten_multidimensional_arrays = false,
            .enable_420pack_extension = true,
            .emit_push_constant_as_uniform_buffer = true,
            .emit_uniform_buffer_as_plain_uniforms = false,
            .emit_line_directives = false,
            .enable_storage_image_qualifier_deduction = true,
            .force_zero_initialized_variables = false,
            .force_flattened_io_blocks = false,
            .relax_nan_checks = false,
            // .enable_row_major_workaround = true,
            .ovr_multiview_view_count = 0,
            .vertex = {
                    .fixup_clipspace = true,
                    .flip_vert_y = false,
                    .support_nonzero_base_instance = true
            },
            .fragment = {
                    .default_float_precision = spirv_cross::CompilerGLSL::Options::Mediump,
                    .default_int_precision = spirv_cross::CompilerGLSL::Options::Highp,
            }
        };

        compiler.set_common_options(options);
        auto crossed = compiler.compile();
        spdlog::trace("Cross-compiled GLSL shader\n{}", crossed);

        switch (stage) {
            case Stage::VERTEX:
                module = glCreateShader(GL_VERTEX_SHADER);
                break;
            case Stage::FRAGMENT:
                module = glCreateShader(GL_FRAGMENT_SHADER);
                break;
            default:
                spdlog::error("Unsupported shader stage");
                throw std::runtime_error("Unsupported shader stage");
        }
        auto src = crossed.c_str();
        glShaderSource(module, 1, &src, nullptr);
        glCompileShader(module);

        GLint status;
        glGetShaderiv(module, GL_COMPILE_STATUS, &status);
        if (!status) {
            GLint size;
            glGetShaderiv(module, GL_INFO_LOG_LENGTH, &size);
            char log[size];
            glGetShaderInfoLog(module, size, nullptr, log);
            spdlog::error("Failed to compile GL shader module: {}", std::string(log));
            throw std::runtime_error("Failed to compile GL shader module.");
        }
    }

    GlShaderModule::~GlShaderModule() {
        glDeleteShader(module);
    }
}
