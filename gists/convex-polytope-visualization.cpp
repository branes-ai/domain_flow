#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <algorithm>

// Structure to represent a hyperplane constraint: ax + by + cz + d <= 0
struct Hyperplane {
    glm::vec4 coefficients; // (a, b, c, d)
};

// Structure to represent a vertex of the polytope
struct Vertex {
    glm::vec3 position;
};

// Structure to represent an edge of the polytope
struct Edge {
    int v1, v2; // Indices of vertices that form the edge
};

// Class to handle the convex polytope
class ConvexPolytope {
private:
    std::vector<Hyperplane> hyperplanes;
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    
    // Bounds for the region where we compute the polytope
    glm::vec3 minBounds;
    glm::vec3 maxBounds;
    
public:
    ConvexPolytope(const glm::vec3& min, const glm::vec3& max) : minBounds(min), maxBounds(max) {}
    
    void addHyperplane(const Hyperplane& plane) {
        hyperplanes.push_back(plane);
    }
    
    // Check if a point satisfies all hyperplane constraints
    bool isPointValid(const glm::vec3& point) const {
        for (const auto& plane : hyperplanes) {
            float result = plane.coefficients.x * point.x + 
                           plane.coefficients.y * point.y + 
                           plane.coefficients.z * point.z + 
                           plane.coefficients.w;
            if (result > 0.0001f) { // Small epsilon for numerical stability
                return false;
            }
        }
        return true;
    }
    
    // Compute the intersection of three hyperplanes
    bool computeIntersection(int p1, int p2, int p3, glm::vec3& point) const {
        glm::mat3 A(hyperplanes[p1].coefficients.x, hyperplanes[p1].coefficients.y, hyperplanes[p1].coefficients.z,
                    hyperplanes[p2].coefficients.x, hyperplanes[p2].coefficients.y, hyperplanes[p2].coefficients.z,
                    hyperplanes[p3].coefficients.x, hyperplanes[p3].coefficients.y, hyperplanes[p3].coefficients.z);
        
        glm::vec3 b(-hyperplanes[p1].coefficients.w,
                     -hyperplanes[p2].coefficients.w,
                     -hyperplanes[p3].coefficients.w);
        
        // Check if the matrix is invertible
        float det = glm::determinant(A);
        if (std::abs(det) < 0.0001f) {
            return false;
        }
        
        // Solve the system Ax = b
        glm::mat3 Ainv = glm::inverse(A);
        point = Ainv * b;
        
        return true;
    }
    
    // Compute the vertices and edges of the polytope
    void computeConvexHull() {
        vertices.clear();
        edges.clear();
        
        // For each triplet of hyperplanes, compute their intersection
        for (size_t i = 0; i < hyperplanes.size(); ++i) {
            for (size_t j = i + 1; j < hyperplanes.size(); ++j) {
                for (size_t k = j + 1; k < hyperplanes.size(); ++k) {
                    glm::vec3 intersection;
                    if (computeIntersection(i, j, k, intersection)) {
                        // Check if the intersection point is valid (satisfies all constraints)
                        // and is within bounds
                        if (isPointValid(intersection) &&
                            intersection.x >= minBounds.x && intersection.x <= maxBounds.x &&
                            intersection.y >= minBounds.y && intersection.y <= maxBounds.y &&
                            intersection.z >= minBounds.z && intersection.z <= maxBounds.z) {
                            
                            // Check if this vertex is duplicate
                            bool isDuplicate = false;
                            for (const auto& v : vertices) {
                                if (glm::distance(v.position, intersection) < 0.0001f) {
                                    isDuplicate = true;
                                    break;
                                }
                            }
                            
                            if (!isDuplicate) {
                                vertices.push_back({intersection});
                            }
                        }
                    }
                }
            }
        }
        
        // Compute edges - this is a simplified approach
        // A more robust approach would use computational geometry libraries
        for (size_t i = 0; i < vertices.size(); ++i) {
            for (size_t j = i + 1; j < vertices.size(); ++j) {
                // Check if vertices i and j form an edge by checking if their midpoint is valid
                glm::vec3 midpoint = (vertices[i].position + vertices[j].position) * 0.5f;
                if (isPointValid(midpoint)) {
                    // Check if this edge lies on exactly two hyperplanes
                    int planes_on = 0;
                    for (const auto& plane : hyperplanes) {
                        float val_i = glm::dot(glm::vec3(plane.coefficients), vertices[i].position) + plane.coefficients.w;
                        float val_j = glm::dot(glm::vec3(plane.coefficients), vertices[j].position) + plane.coefficients.w;
                        if (std::abs(val_i) < 0.0001f && std::abs(val_j) < 0.0001f) {
                            planes_on++;
                        }
                    }
                    
                    if (planes_on >= 2) {
                        edges.push_back({static_cast<int>(i), static_cast<int>(j)});
                    }
                }
            }
        }
    }
    
    const std::vector<Vertex>& getVertices() const {
        return vertices;
    }
    
    const std::vector<Edge>& getEdges() const {
        return edges;
    }
};

// Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main() {
    FragColor = vec4(color, 1.0);
}
)";

// Function to compile and link shaders
unsigned int createShaderProgram() {
    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Convex Polytope Visualization", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // Set viewport
    glViewport(0, 0, 800, 600);
    
    // Create and compile shaders
    unsigned int shaderProgram = createShaderProgram();
    
    // Define a convex polytope with hyperplane constraints
    ConvexPolytope polytope(glm::vec3(-10.0f), glm::vec3(10.0f));
    
    // Example: Add some hyperplane constraints to define a cube
    // x >= -1 => -x - 1 <= 0
    polytope.addHyperplane({glm::vec4(-1.0f, 0.0f, 0.0f, -1.0f)});
    // x <= 1 => x - 1 <= 0
    polytope.addHyperplane({glm::vec4(1.0f, 0.0f, 0.0f, -1.0f)});
    // y >= -1 => -y - 1 <= 0
    polytope.addHyperplane({glm::vec4(0.0f, -1.0f, 0.0f, -1.0f)});
    // y <= 1 => y - 1 <= 0
    polytope.addHyperplane({glm::vec4(0.0f, 1.0f, 0.0f, -1.0f)});
    // z >= -1 => -z - 1 <= 0
    polytope.addHyperplane({glm::vec4(0.0f, 0.0f, -1.0f, -1.0f)});
    // z <= 1 => z - 1 <= 0
    polytope.addHyperplane({glm::vec4(0.0f, 0.0f, 1.0f, -1.0f)});
    
    // Compute the convex hull
    polytope.computeConvexHull();
    
    // Prepare vertex data for OpenGL
    const std::vector<Vertex>& vertices = polytope.getVertices();
    const std::vector<Edge>& edges = polytope.getEdges();
    
    std::vector<float> vertexData;
    for (const auto& vertex : vertices) {
        vertexData.push_back(vertex.position.x);
        vertexData.push_back(vertex.position.y);
        vertexData.push_back(vertex.position.z);
    }
    
    std::vector<unsigned int> indices;
    for (const auto& edge : edges) {
        indices.push_back(edge.v1);
        indices.push_back(edge.v2);
    }
    
    // Create VAO, VBO, and EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Create transformations
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(3.0f, 3.0f, 3.0f), // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
        );
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        
        // Pass transformation matrices to the shader
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        int colorLoc = glGetUniformLocation(shaderProgram, "color");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);
        
        // Draw points (vertices)
        glBindVertexArray(VAO);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, vertices.size());
        
        // Draw lines (edges)
        glUniform3f(colorLoc, 0.0f, 1.0f, 1.0f);
        glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}
