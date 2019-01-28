

#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class ShaderType {
	RENDER,
	COMPUTE
};

template <ShaderType SHADER_TYPE = ShaderType::RENDER>
class Shader
{
public:
	unsigned int ID;
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char * fShaderCode = fragmentCode.c_str();
		// 2. compile shaders
		unsigned int vertex, fragment;
		// vertex shader
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}


	// activate the shader
	// ------------------------------------------------------------------------
	void use()
	{
		glUseProgram(ID);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}


	template <int N>
	void setVector(const std::string &name, const glm::vec<N, float, glm::packed_highp>& vec)
	{
		switch (N) {
		case 2: {
			glUniform2fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		case 3: {
			glUniform3fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		case 4: {
			glUniform4fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		}
	}

	template <int N>
	void setMatrix(const std::string &name, const glm::mat<N, N, float, glm::packed_highp>& vec)
	{
		switch (N) {
		case 2: {
			glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		case 3: {
			glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		case 4: {
			glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), vec);
		}; break;
		}
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(unsigned int shader, std::string type)
	{
		int success;
		char infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};

template <>
class Shader<ShaderType::COMPUTE> {
private:
	unsigned int ID;

public:
	Shader(const char* computePath) {
		std::string computeCode;
		std::ifstream cShaderFile;
		// ensure ifstream objects can throw exceptions:
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			cShaderFile.open(computePath);
			std::stringstream cShaderStream;
			// read file's buffer contents into streams
			cShaderStream << cShaderFile.rdbuf();

			// close file handlers
			cShaderFile.close();
			// convert stream into string
			computeCode = cShaderStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* cShaderCode = computeCode.c_str();
		// 2. compile shaders
		unsigned int compute;
		// vertex shader
		compute = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(compute, 1, &cShaderCode, NULL);
		glCompileShader(compute);
		checkCompileErrors(compute, "COMPUTE");
		// shader Program
		ID = glCreateProgram();
		glAttachShader(ID, compute);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		glDeleteShader(compute);
	}

	void use()
	{
		glUseProgram(ID);
	}

	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string &name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string &name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}

	template <int N>
	void setVector(const std::string &name, const glm::vec<N, float, glm::packed_highp>& vec)
	{
		switch (N) {
		case 2: {
			glUniform2fv(glGetUniformLocation(ID, name.c_str()), vec.length(), glm::value_ptr(vec));
		}; break;
		case 3: {
			glUniform3fv(glGetUniformLocation(ID, name.c_str()), vec.length(), glm::value_ptr(vec));
		}; break;
		case 4: {
			glUniform4fv(glGetUniformLocation(ID, name.c_str()), vec.length(), glm::value_ptr(vec));
		}; break;
		}
	}

	template <int N>
	void setMatrix(const std::string &name, const glm::mat<N, N, float, glm::packed_highp>& vec)
	{
		switch (N) {
		case 2: {
			glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), vec.length(), GL_FALSE, glm::value_ptr(vec));
		}; break;
		case 3: {
			glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), vec.length(), GL_FALSE, glm::value_ptr(vec));
		}; break;
		case 4: {
			glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), vec.length(), GL_FALSE, glm::value_ptr(vec));
		}; break;
		}
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(unsigned int shader, std::string type)
	{
		int success;
		char infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};

#endif

