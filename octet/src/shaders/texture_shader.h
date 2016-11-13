////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Single texture shader with no lighting

namespace octet { namespace shaders {
  class texture_shader : public shader {
    // indices to use with glUniform*()

    // index for model space to projection space matrix
    GLuint modelToProjectionIndex_;

    // index for texture sampler
    GLuint samplerIndex_;

	GLuint color_index;
	//vec4 color_vec;

  public:
    void init() {
      // this is the vertex shader.
      // it is called for each corner of each triangle
      // it inputs pos and uv from each corner
      // it outputs gl_Position and uv_ to the rasterizer
      const char vertex_shader[] = SHADER_STR(
        varying vec2 uv_;

        attribute vec4 pos;
        attribute vec2 uv;

        uniform mat4 modelToProjection;
		
        void main() { gl_Position = modelToProjection * pos; uv_ = uv; }
      );

      // this is the fragment shader
      // after the rasterizer breaks the triangle into fragments
      // this is called for every fragment
      // it outputs gl_FragColor, the color of the pixel and inputs uv_
	  //color_vec = color;
      const char fragment_shader[] = SHADER_STR(
        varying vec2 uv_;
	    //attribute vec4 color
        uniform sampler2D sampler;

		uniform vec4 color;
        //void main() { gl_FragColor = texture2D(sampler, uv_); }
		void main() { gl_FragColor = texture2D(sampler, uv_)* color; }
      );
   
      // use the common shader code to compile and link the shaders
      // the result is a shader program
      shader::init(vertex_shader, fragment_shader);

      // extract the indices of the uniforms to use later
      modelToProjectionIndex_ = glGetUniformLocation(program(), "modelToProjection");
      samplerIndex_ = glGetUniformLocation(program(), "sampler");
	  color_index = glGetUniformLocation(program(), "color");
    }
	//, float color[4]
    void render(const mat4t &modelToProjection, int sampler, vec4 _color) {
      // tell openGL to use the program
      shader::render();
	  
	  //pass values in vec4 to array to match the format of glUniform4fv
	  float colour_array[4];
	  colour_array[0] = _color.x();
	  colour_array[1] = _color.y();
	  colour_array[2] = _color.z();
	  colour_array[3] = _color.w();

      // customize the program with uniforms
      glUniform1i(samplerIndex_, sampler);
      glUniformMatrix4fv(modelToProjectionIndex_, 1, GL_FALSE, modelToProjection.get());
	  glUniform4fv(color_index, 1, colour_array);
    }
  };
}}
