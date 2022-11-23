#pragma once
#include <string>

    class Program {
    public:
        Program();

        ~Program();

        Program& operator = (Program&&) noexcept = delete;
        Program& operator = (const Program&) = delete;
        Program(const Program&) = delete;
        Program(Program&&) noexcept = delete;

        int getAttribLocation(const std::string& name);
        int getUniformLocation(const std::string& name);

        bool create(const std::string& vertexSource, const std::string& fragmentSource);

        void use();
        void unuse();
        void release();

    protected:

        unsigned int id;
    };
