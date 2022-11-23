#include <glad/glad.h>
#include <program.h>
#include <sstream>
#include <iostream>

namespace {
    unsigned int compileShader(unsigned int type, const char* source) {
        unsigned int shader = glCreateShader(type);
        if (!shader) {
            //todo::error  info
            return 0;
        }

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint compileStatus = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
        if (!compileStatus) {
            GLchar info[512];
            GLsizei length = 0;
            glGetShaderInfoLog(shader, 512, &length, info);
            glDeleteShader(shader);
            std::cout << info << std::endl;
            return 0;
        }

        return shader;
    }
}

    Program::Program() : id(0) {
    }

    Program::~Program() {
        release();
    }

    int Program::getAttribLocation(const std::string& name) {
        return glGetAttribLocation(id, name.c_str());
    }

    int Program::getUniformLocation(const std::string& name) {
        return glGetUniformLocation(id, name.c_str());
    }

    bool Program::create(const std::string& vertexSource, const std::string& fragmentSource) {
        GLuint programId = glCreateProgram();
        if (!programId) {
            return false;
        }

        unsigned int vertexShader   = compileShader(GL_VERTEX_SHADER, vertexSource.c_str());
        if (!vertexShader) {
            return false;
        }

        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());
        if (!fragmentShader) {
            return false;
        }

        glAttachShader(programId, vertexShader);
        glAttachShader(programId, fragmentShader);
        glLinkProgram(programId);

        GLint status;
        glGetProgramiv(programId, GL_LINK_STATUS, &status);
        if (!status) {
            GLchar info[512];
            glGetProgramInfoLog(programId, 512, nullptr, info);

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteProgram(programId);
            return false;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        id = programId;
        return true;
    }

    void Program::use() {
        glUseProgram(id);
    }

    void Program::unuse() {
        glUseProgram(0);
    }

    void Program::release() {
        glDeleteProgram(id);
        id = 0;
    }