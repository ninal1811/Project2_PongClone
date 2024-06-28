/*
* Author: Nina Li
* Assignment: Pong Clone
* Date due: 2024-06-29, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
*/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>

enum AppStatus { RUNNING, TERMINATED };

constexpr float WINDOW_SIZE_MULT = 1.0f;

constexpr int WINDOW_WIDTH = 640 * WINDOW_SIZE_MULT,
              WINDOW_HEIGHT = 480 * WINDOW_SIZE_MULT;

constexpr float BG_RED = 0.0f,
                BG_GREEN = 0.0f,
                BG_BLUE = 0.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
              VIEWPORT_Y = 0,
              VIEWPORT_WIDTH = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr char LEFT_SPRITE_FILEPATH[] = "left.png",
               RIGHT_SPRITE_FILEPATH[] = "right.png",
               STAR_SPRITE_FILEPATH[] = "star.png",
               WIN1_SPRITE_FILEPATH[] = "player1message.png",
               WIN2_SPRITE_FILEPATH[] = "player2message.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(1.14f, 1.14f, 0.0f),
                    INIT_POS_LEFT = glm::vec3(-4.0f, 0.0f, 0.0f),
                    INIT_POS_RIGHT = glm::vec3(4.0f, 0.0f, 0.0f),
                    INIT_SCALE_WIN = glm::vec3(6.32f, 1.92f, 0.0f);

SDL_Window* g_display_window;

float g_paddle_speed = 1.0f;
bool single_player = false;
float auto_movement = 1.0f;
float g_star_speed = 1.2f;
bool game_over = false;
bool player1_wins = false;
constexpr int MAX_STARS = 3;
int num_stars = 1;

AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_left_matrix, g_projection_matrix, g_trans_matrix,
          g_right_matrix, g_star_matrix[MAX_STARS], g_win1_matrix, g_win2_matrix;

float g_previous_ticks = 0.0f;

GLuint g_left_texture_id, g_right_texture_id, g_star_texture_id[MAX_STARS], g_win1_texture_id, g_win2_texture_id;

glm::vec3 g_left_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_left_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_right_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_right_movement = glm::vec3(0.0f, 0.0f, 0.0f);

glm::vec3 g_star_position[MAX_STARS] = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f)
};

glm::vec3 g_star_movement[MAX_STARS] = {
    glm::vec3(1.0f, 1.0f, 0.0f),
    glm::vec3(1.0f, 1.0f, 0.0f),
    glm::vec3(1.0f, 1.0f, 0.0f)
};

void initialise();
void process_input();
void update();
void render();
void shutdown();

constexpr GLint NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

GLuint load_texture(const char* filepath) {
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL) {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return textureID;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Project 2: Pong Clone!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    if (g_display_window == nullptr) {
        shutdown();
    }
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_left_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::translate(g_right_matrix, glm::vec3(1.0f, 1.0f, 0.0f));
    g_right_position += g_right_movement;
    
    for (int i = 0; i < MAX_STARS; i++) {
        g_star_matrix[i] = glm::mat4(1.0f);
    }
    
    g_win1_matrix = glm::mat4(1.0f);
    g_win2_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_left_texture_id = load_texture(LEFT_SPRITE_FILEPATH);
    g_right_texture_id = load_texture(RIGHT_SPRITE_FILEPATH);
    g_win1_texture_id = load_texture(WIN1_SPRITE_FILEPATH);
    g_win2_texture_id = load_texture(WIN2_SPRITE_FILEPATH);
    
    for (int i = 0; i < MAX_STARS; i++) {
        g_star_texture_id[i] = load_texture(STAR_SPRITE_FILEPATH);
    }
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    g_left_movement = glm::vec3(0.0f);
    g_right_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                                                                             
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                    case SDLK_t:
                        single_player = !single_player;
                        break;
                    case SDLK_1:
                        num_stars = 1;
                        break;
                    case SDLK_2:
                        num_stars = 2;
                        break;
                    case SDLK_3:
                        num_stars = 3;
                        break;
                                                                             
                    default:
                        break;
                }
                                                                             
            default:
                break;
        }
    }
    
    if (!single_player) {
        const Uint8 *key_state = SDL_GetKeyboardState(NULL);
        if (key_state[SDL_SCANCODE_W]) {
            g_left_movement.y = 1.0f;
        } else if (key_state[SDL_SCANCODE_S]) {
            g_left_movement.y = -1.0f;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_UP]) {
        g_right_movement.y = 1.0f;
    } else if (key_state[SDL_SCANCODE_DOWN]) {
        g_right_movement.y = -1.0f;
    }
                                                                             
    if (glm::length(g_left_movement) > 1.0f) {
        g_left_movement = glm::normalize(g_left_movement);
    }
    
    if (glm::length(g_right_movement) > 1.0f) {
        g_right_movement = glm::normalize(g_right_movement);
    }
}

void update() {
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    if (single_player) {
        g_left_position.y += auto_movement * g_paddle_speed * delta_time;
        if (g_left_position.y + INIT_POS_LEFT.y >= 3.75f - (INIT_SCALE.y / 2.0f) ||
            g_left_position.y + INIT_POS_LEFT.y <= -3.75f + (INIT_SCALE.y / 2.0f)) {
            auto_movement = -auto_movement;
        }
    }

    g_right_position += g_right_movement * g_paddle_speed * delta_time;
    if (!single_player) {
        g_left_position += g_left_movement * g_paddle_speed * delta_time;
    }
        
    if (g_right_position.y + INIT_POS_RIGHT.y > 3.75f - (INIT_SCALE.y / 2.0f)) {
        g_right_position.y = 3.75f - (INIT_SCALE.y / 2.0f) - INIT_POS_RIGHT.y;
    } else if (g_right_position.y + INIT_POS_RIGHT.y < -3.75f + (INIT_SCALE.y / 2.0f)) {
        g_right_position.y = -3.75f + (INIT_SCALE.y / 2.0f) - INIT_POS_RIGHT.y;
    }
        
    if (!single_player) {
        if (g_left_position.y + INIT_POS_LEFT.y > 3.75f - (INIT_SCALE.y / 2.0f)) {
            g_left_position.y = 3.75f - (INIT_SCALE.y / 2.0f) - INIT_POS_LEFT.y;
        } else if (g_left_position.y + INIT_POS_LEFT.y < -3.75f + (INIT_SCALE.y / 2.0f)) {
            g_left_position.y = -3.75f + (INIT_SCALE.y / 2.0f) - INIT_POS_LEFT.y;
        }
    }
    
    for (int i = 0; i <num_stars; i++) {
        g_star_position[i] += g_star_movement[i] * g_paddle_speed * delta_time;
        
        float left_distance = fabs(g_star_position[i].y - (g_left_position.y + INIT_POS_LEFT.y)) - ((INIT_SCALE.y / 2.0f) + 0.05f);
        float right_distance = fabs(g_star_position[i].y - (g_right_position.y + INIT_POS_RIGHT.y)) - ((INIT_SCALE.y / 2.0f) + 0.05f);
            
        if (left_distance < 0.0f && fabs(g_star_position[i].x - (INIT_POS_LEFT.x + g_left_position.x)) < 0.05f) {
            g_star_movement[i].x = fabs(g_star_movement[i].x);
        }
            
        if (right_distance < 0.0f && fabs(g_star_position[i].x - (INIT_POS_RIGHT.x + g_right_position.x)) < 0.05f) {
            g_star_movement[i].x = -fabs(g_star_movement[i].x);
        }
            
        if (g_star_position[i].x <= -5.0f) {
            game_over = true;
            player1_wins = true;
        } else if (g_star_position[i].x >= 5.0f) {
            game_over = true;
            player1_wins = false;
        }
            
        if (g_star_position[i].y >= 3.75f - (INIT_SCALE.y / 2.0f) || g_star_position[i].y <= -3.75f + (INIT_SCALE.y / 2.0f)) {
            g_star_movement[i].y = -g_star_movement[i].y;
        }
    }
        
    g_left_matrix = glm::mat4(1.0f);
    g_left_matrix = glm::translate(g_left_matrix, INIT_POS_LEFT);
    g_left_matrix = glm::translate(g_left_matrix, g_left_position);
    g_left_matrix = glm::scale(g_left_matrix, INIT_SCALE);
        
    g_right_matrix = glm::mat4(1.0f);
    g_right_matrix = glm::translate(g_right_matrix, INIT_POS_RIGHT);
    g_right_matrix = glm::translate(g_right_matrix, g_right_position);
    g_right_matrix = glm::scale(g_right_matrix, INIT_SCALE);
        
    for (int i = 0; i < num_stars; i++) {
        g_star_matrix[i] = glm::mat4(1.0f);
        g_star_matrix[i] = glm::translate(g_star_matrix[i], g_star_position[i]);
    }
    
    float x_distance = fabs(g_left_position.x + INIT_POS_LEFT.x - INIT_POS_RIGHT.x) - ((INIT_SCALE.x + INIT_SCALE.x) / 2.0f);

    float y_distance = fabs(g_left_position.y + INIT_POS_LEFT.y - INIT_POS_RIGHT.y) - ((INIT_SCALE.y + INIT_SCALE.y) / 2.0f);
    
    if (x_distance < 0.0f && y_distance < 0.0f) {
        std::cout << std::time(nullptr) << ": Collision.\n";
    }
    
    if (game_over) {
        if (player1_wins) {
            g_win2_matrix = glm::mat4(1.0f);
            g_win2_matrix = glm::scale(g_win2_matrix, INIT_SCALE_WIN);
        } else {
            g_win1_matrix = glm::mat4(1.0f);
            g_win1_matrix = glm::scale(g_win1_matrix, INIT_SCALE_WIN);
        }
    }
}

void draw_object(glm::mat4 &object_model_matrix, GLuint &object_texture_id) {
    g_shader_program.set_model_matrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_left_matrix, g_left_texture_id);
    draw_object(g_right_matrix, g_right_texture_id);
    
    for (int i = 0; i < num_stars; i++) {
        draw_object(g_star_matrix[i], g_star_texture_id[i]);
    }
    
    if (game_over) {
        for (int i = 0; i < num_stars; i++) {
            g_star_position[i] = glm::vec3(0.0f, 0.0f, 0.0f);
            g_star_movement[i] = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        
        if (player1_wins) {
            draw_object(g_win2_matrix, g_win2_texture_id);
        } else {
            draw_object(g_win1_matrix, g_win1_texture_id);
        }
    }
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[]) {
    initialise();
    
    while (g_app_status == RUNNING) {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}

