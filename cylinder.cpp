#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cmath>
#include <vector>
#include <glm/vector_relational.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <chrono>
#include "stb_image.h"

#define WIDTH 1280
#define HEIGHT 720


const GLchar *vertex_source = R"(
#version 150

in vec3 position;
in vec2 texture_coordinate;
out vec2 TextureCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

void main(){
 TextureCoord = texture_coordinate;

 gl_Position = projection * view * model * vec4(position,1.0);
}
)";


const GLchar *fragment_source = R"(
#version 150
in vec2 TextureCoord;
uniform sampler2D tex;
out vec4 out_Texture;
void main(){

out_Texture = texture(tex, TextureCoord);
}
)";


struct coordinate{
  float x,y,z; 
  float u,v;
};


float yaw = -90.0f;
float pitch = 0.0f;

void mouse_movement(SDL_Event& mouseEvent, glm::vec3& cameraFront_upping){

        float xoffset = static_cast<float>(mouseEvent.motion.xrel); 
        float yoffset = -static_cast<float>(mouseEvent.motion.yrel); 

        float sensitivity = 0.1f;         

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)  pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront_upping = glm::normalize(front);
}


int main(){
  
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("cylinder shader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH,HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
 glewExperimental = GL_TRUE;
  
  glewInit();

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glEnable(GL_DEPTH_TEST);
  
  //the slices of cylinder
  int slices = 36;
  float radius = 0.7f;
  float cylinder_height = 1.0f;


  std::vector<coordinate> vertices;

  int circle_indices = slices * 3 * 2;
  int rectangle_indices = slices * 6;
  int cylinder_indices = circle_indices + rectangle_indices;
  unsigned int *indices = new unsigned int[cylinder_indices];

  float top = cylinder_height/2;
  float bottom = -top;


  //top side of cylinder
  vertices.push_back({0.0f,top, 0.0f, 0.5f, 1.0f});

  for(int i = 0; i<slices; i++){
  float theta = i * 2 * M_PI / slices;

  
  float x = cos(theta) * radius;
  float y = top;
  float z = sin(theta) * radius;

  float u = (cos(theta) * 0.5f) + 0.5f;
  float v = 1.0f - (sin(theta) * 0.5f);

  vertices.push_back({x,y,z,u,v});
  
  int index = i * 3;

  indices[index] = 0;
  indices[index+1] = i+1;

  if(i == slices - 1){
    indices[index+2] = 1;
  }
  else { 
  indices[index+2] = i+2;
  }
  }

  //bottom side of cylinder 
  unsigned int bottom_plane = vertices.size();

  vertices.push_back({0.0f,bottom, 0.0f, 0.5f, 1.0f});
  for(int i = 0; i<slices; i++){
  float theta = i * 2 * M_PI / slices;

  
  float x = cos(theta) * radius;
  float y = bottom;
  float z = sin(theta) * radius;

  float u = (cos(theta) * 0.5f) + 0.5f;
  float v = 1.0f - (sin(theta) * 0.5f);

  vertices.push_back({x,y,z,u,v});
  
  int index = (slices*3)+(i * 3);

  indices[index] = bottom_plane;
  indices[index+2] = bottom_plane+i+1;

  if(i == slices - 1){
    indices[index+1] = bottom_plane+1;
  }
  else { 
  indices[index+1] = bottom_plane+i+2;
  }
  }

  for(int i = 0; i<slices; i++){
    unsigned int top_left = i+1;
    unsigned int top_right = (i == slices - 1) ? 1 : i+2;

    unsigned int bottom_left = bottom_plane+i+1;
    unsigned int bottom_right = (i == slices - 1) ? bottom_plane+1 : bottom_plane+i+2;
    
    int idx = circle_indices + (i*6);

    indices[idx] = top_left;
    indices[idx+1] = top_right;
    indices[idx+2] = bottom_left;
    
    indices[idx+3] = bottom_left;
    indices[idx+4] = bottom_right;
    indices[idx+5] = top_right;

    
  }




  GLuint vao, vbo, ebo;
  glGenVertexArrays(1,&vao);
  glBindVertexArray(vao);

  glGenBuffers(1,&vbo);
  glBindBuffer(GL_ARRAY_BUFFER,vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(coordinate), vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1,&ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinder_indices*sizeof(unsigned int), indices, GL_STATIC_DRAW);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader,1, &vertex_source,NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader,1, &fragment_source,NULL);
  glCompileShader(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program,vertex_shader);
  glAttachShader(shader_program,fragment_shader);
  glBindFragDataLocation(shader_program,0,"out_Texture");
  glLinkProgram(shader_program);
  glUseProgram(shader_program);

  GLint position_attribute = glGetAttribLocation(shader_program, "position");
  glEnableVertexAttribArray(position_attribute);
  glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, sizeof(coordinate),0);

  GLint texture_attribute = glGetAttribLocation(shader_program, "texture_coordinate");
  glEnableVertexAttribArray(texture_attribute);
  glVertexAttribPointer(texture_attribute, 2, GL_FLOAT, GL_FALSE, sizeof(coordinate),(void*)(3*sizeof(GLfloat)));

  GLint uniModel = glGetUniformLocation(shader_program, "model");
  GLint uniView = glGetUniformLocation(shader_program, "view");
  GLint uniProjection = glGetUniformLocation(shader_program, "projection");

  int x;
  int y;
  int channel_img;
  unsigned char *image = stbi_load("/home/moel/Pictures/metal color.png", &x, &y, &channel_img, 4);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
    
    auto last_time = std::chrono::high_resolution_clock::now();
    SDL_SetRelativeMouseMode(SDL_TRUE);


  bool running = true;
  
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glm::vec3 cameraPos = glm::vec3(0.0f,0.0f,3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f,0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);

  SDL_Event event;
  while(running){
    auto time_now = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_delta = time_now - last_time;
    float deltaTime = elapsed_delta.count();
    last_time = time_now;

    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT){
        running = false;
      }
      if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN){
        running = false;
      }
      if(event.type == SDL_MOUSEMOTION){
          mouse_movement(event, cameraFront);
      }
    }
      
    const Uint8* state = SDL_GetKeyboardState(NULL);
    const float cameraSpeed = 10.0f * deltaTime;

      if (state[SDL_SCANCODE_W]) {
        cameraPos += cameraSpeed * cameraFront;
      }
      if (state[SDL_SCANCODE_S]) {
        cameraPos -= cameraSpeed * cameraFront;
      }

      if (state[SDL_SCANCODE_A]) {
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
      }
      if (state[SDL_SCANCODE_D]) {
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
      }

    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = glm::lookAt(cameraPos,  cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(uniView,1,GL_FALSE,glm::value_ptr(view));

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/ 720.0f, 1.0f, 1000.0f);
    glUniformMatrix4fv(uniProjection,1,GL_FALSE,glm::value_ptr(proj));
    
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

    glDrawElements(GL_TRIANGLES, cylinder_indices, GL_UNSIGNED_INT, 0);


  SDL_GL_SwapWindow(window);
  }
  
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);

  glDeleteProgram(shader_program);
  glDeleteShader(fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteTextures(1, &tex);
  glDeleteBuffers(1, &ebo);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
  delete[] indices;
    SDL_Quit();
}
