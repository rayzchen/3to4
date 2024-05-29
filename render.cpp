/**************************************************************************
 * 3to4++ - https://github.com/rayzchen/3to4++
 *-------------------------------------------------------------------------
 * Copyright 2024 Ray Chen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 **************************************************************************/

#include <glad/gl.h>
#include "render.h"
#include "constants.h"
#include <iostream>
#include <array>
#include <algorithm>

void mat4x4_scale_pos(mat4x4 M, float k) {
    for (int i = 0; i < 3; i++) {
        M[3][i] *= k;
    }
}

float smoothstep(float t) {
    return t * t * (3 - 2 * t);
}

PieceMesh::PieceMesh(PieceType type) {
    length1 = type.triangles.size();
    length2 = type.edges.size();
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &faceVao);
    glGenVertexArrays(1, &edgeVao);
    glGenBuffers(1, &faceEbo);
    glGenBuffers(1, &edgeEbo);

    glBindVertexArray(faceVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, type.vertices.size() * sizeof(float), type.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, type.triangles.size() * sizeof(unsigned int), type.triangles.data(), GL_STATIC_DRAW);
    // 3 floats for XYZ, 1 float for color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(edgeVao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, type.vertices.size() * sizeof(float), type.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgeEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, type.edges.size() * sizeof(unsigned int), type.edges.data(), GL_STATIC_DRAW);
    // 3 floats for XYZ, 1 float for color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void PieceMesh::renderFaces() {
    glBindVertexArray(faceVao);
    glDrawElements(GL_TRIANGLES, length1, GL_UNSIGNED_INT, 0);
}

void PieceMesh::renderEdges() {
    glBindVertexArray(edgeVao);
    glDrawElements(GL_LINES, length2, GL_UNSIGNED_INT, 0);
}

Shader::Shader(const char *vertex, const char *fragment) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex, NULL);
    glCompileShader(vertexShader);
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::use() {
    glUseProgram(program);
}

void Shader::setInt(const char *loc, int value) {
    int shaderLocation = glGetUniformLocation(program, loc);
    glUniform1i(shaderLocation, value);
}

void Shader::setVec3(const char *loc, vec3 vector) {
    int shaderLocation = glGetUniformLocation(program, loc);
    glUniform3fv(shaderLocation, 1, vector);
}

void Shader::setMat4(const char *loc, mat4x4 matrix) {
    int shaderLocation = glGetUniformLocation(program, loc);
    glUniformMatrix4fv(shaderLocation, 1, GL_FALSE, matrix[0]);
}

Shader::~Shader() {
    glDeleteProgram(program);
}

PuzzleRenderer::PuzzleRenderer(Puzzle *puzzle) {
    this->puzzle = puzzle;
    spacing = 0.0f;
    sensitivity = 0.01f;
    animating = false;
    animationProgress = 0.0f;
    animationSpeed = 4.0f;
    mat4x4_identity(model);

    meshes[0] = new PieceMesh(Pieces::mesh1c);
    meshes[1] = new PieceMesh(Pieces::mesh2c);
    meshes[2] = new PieceMesh(Pieces::mesh3c);
    meshes[3] = new PieceMesh(Pieces::mesh4c);
}

PuzzleRenderer::~PuzzleRenderer() {
    for (int i = 0; i < 4; i++) {
        delete meshes[i];
    }
}

float PuzzleRenderer::getSpacing() {
    return spacing;
}

void PuzzleRenderer::setSpacing(float spacing) {
    this->spacing = spacing;
    if (this->spacing < 0.0f) {
        this->spacing = 0.0f;
    } else if (this->spacing > 1.5f) {
        this->spacing = 1.5f;
    }
}

void PuzzleRenderer::render1c(Shader *shader, const std::array<float, 3> pos, Color color) {
    shader->use();
    shader->setVec3("pieceColors[0]", Pieces::colors[color]);

    float scale = getSpacing() + 1.0f;
    mat4x4 model;
    mat4x4_dup(model, this->model);
    mat4x4_translate_in_place(model, pos[0], pos[1], pos[2]);
    mat4x4_scale_pos(model, scale);
    shader->setMat4("model", model);

    shader->setInt("border", 0);
    meshes[0]->renderFaces();
    shader->setInt("border", 1);
    meshes[0]->renderEdges();
}

void PuzzleRenderer::render2c(Shader *shader, const std::array<float, 3> pos, const std::array<Color, 2> colors, CellLocation dir) {
    shader->use();
    shader->setVec3("pieceColors[0]", Pieces::colors[colors[0]]);
    shader->setVec3("pieceColors[1]", Pieces::colors[colors[1]]);

    float scale = getSpacing() + 1.0f;
    mat4x4 model;
    mat4x4_dup(model, this->model);
    mat4x4_translate_in_place(model, pos[0], pos[1], pos[2]);
    mat4x4_scale_pos(model, scale);

    switch (dir) {
        case UP: break;
        case DOWN: mat4x4_rotate(model, model, 1, 0, 0, M_PI); break;
        case RIGHT: mat4x4_rotate(model, model, 0, 0, 1, -M_PI_2); break;
        case LEFT: mat4x4_rotate(model, model, 0, 0, 1, M_PI_2); break;
        case FRONT: mat4x4_rotate(model, model, 1, 0, 0, M_PI_2); break;
        case BACK: mat4x4_rotate(model, model, 1, 0, 0, -M_PI_2); break;
        case IN:
        case OUT:
            return;
    }

    shader->setMat4("model", model);

    shader->setInt("border", 0);
    meshes[1]->renderFaces();
    shader->setInt("border", 1);
    meshes[1]->renderEdges();
}

void PuzzleRenderer::render3c(Shader *shader, const std::array<float, 3> pos, const std::array<Color, 3> colors) {
    shader->use();
    shader->setVec3("pieceColors[0]", Pieces::colors[colors[0]]);
    shader->setVec3("pieceColors[1]", Pieces::colors[colors[1]]);
    shader->setVec3("pieceColors[2]", Pieces::colors[colors[2]]);

    float scale = getSpacing() + 1.0f;
    mat4x4 model;
    mat4x4_dup(model, this->model);
    mat4x4_translate_in_place(model, pos[0], pos[1], pos[2]);
    mat4x4_scale_pos(model, scale);
    shader->setMat4("model", model);

    shader->setInt("border", 0);
    meshes[2]->renderFaces();
    shader->setInt("border", 1);
    meshes[2]->renderEdges();
}

void PuzzleRenderer::render4c(Shader *shader, const std::array<float, 3> pos, const std::array<Color, 4> colors, int orientation) {
    shader->use();
    shader->setVec3("pieceColors[0]", Pieces::colors[colors[0]]);
    shader->setVec3("pieceColors[1]", Pieces::colors[colors[1]]);
    shader->setVec3("pieceColors[2]", Pieces::colors[colors[2]]);
    shader->setVec3("pieceColors[3]", Pieces::colors[colors[3]]);

    float scale = getSpacing() + 1.0f;
    mat4x4 model;
    mat4x4_dup(model, this->model);
    mat4x4_translate_in_place(model, pos[0], pos[1], pos[2]);
    mat4x4_scale_pos(model, scale);
    if (orientation > 3) {
        orientation -= 4;
        mat4x4_rotate(model, model, 1, 0, 0, M_PI);
        mat4x4_rotate(model, model, 0, 1, 0, -M_PI_2 * (orientation - 1));
    } else {
        mat4x4_rotate(model, model, 0, 1, 0, M_PI_2 * orientation);
    }
    shader->setMat4("model", model);

    shader->setInt("border", 0);
    meshes[3]->renderFaces();
    shader->setInt("border", 1);
    meshes[3]->renderEdges();
}

void PuzzleRenderer::updateMouse(GLFWwindow* window, double dt) {
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE);
    if (state == GLFW_PRESS) {
        double curX, curY;
        glfwGetCursorPos(window, &curX, &curY);
        if (lastY != -1.0f) {
            setSpacing(spacing - (curY - lastY) * sensitivity);
        }
        lastY = curY;
    } else {
        lastY = -1.0f;
    }
}

bool checkFilter(std::array<int, 2> filter, std::array<int, 2> pos) {
    return ((filter[0] == -1 || filter[0] == pos[0] + 1) &&
            (filter[1] == -1 || filter[1] == pos[1] + 1));
}

bool checkFilter(std::array<int, 3> filter, std::array<int, 3> pos) {
    return ((filter[0] == -1 || filter[0] == pos[0] + 1) &&
            (filter[1] == -1 || filter[1] == pos[1] + 1) &&
            (filter[2] == -1 || filter[2] == pos[2] + 1));
}

void PuzzleRenderer::renderCell(Shader *shader, const std::array<std::array<std::array<Piece, 3>, 3>, 3>& cell, float offset, std::array<int, 3> sliceFilter) {
    if (checkFilter(sliceFilter, {0, 0, 0})) {
        render1c(shader, {offset, 0, 0}, cell[1][1][1].a);
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            std::array<int, 3> pos = {0, 0, 0};
            pos[i] += j * -2 + 1;
            if (checkFilter(sliceFilter, pos)) {
                Piece piece = cell[pos[0] + 1][pos[1] + 1][pos[2] + 1];
                CellLocation orientation = (CellLocation)(i * 2 + j + 2);
                render2c(shader, {(float)pos[0] + offset, (float)pos[1], (float)pos[2]}, {piece.a, piece.b}, orientation);
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                std::array<int, 3> pos = {0, 0, 0};
                pos[i] = k * -2 + 1;
                pos[(i + 1) % 3] = j * -2 + 1;
                if (checkFilter(sliceFilter, pos)) {
                    Piece piece = cell[pos[0] + 1][pos[1] + 1][pos[2] + 1];
                    render3c(shader, {(float)pos[0] + offset, (float)pos[1], (float)pos[2]}, {piece.a, piece.b, piece.c});
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                std::array<int, 3> pos = {i * -2 + 1, j * -2 + 1, k * -2 + 1};
                int orientation = (i + k) + 2*i*(1 - k);
                if (checkFilter(sliceFilter, pos)) {
                    Piece piece = cell[pos[0] + 1][pos[1] + 1][pos[2] + 1];
                    render4c(shader, {(float)pos[0] + offset, (float)pos[1], (float)pos[2]}, {piece.a, piece.b, piece.c, piece.d}, 4*j + orientation);
                }
            }
        }
    }
}

void PuzzleRenderer::renderSlice(Shader *shader, const std::array<std::array<Piece, 3>, 3>& slice, float offset, std::array<int, 2> stripFilter) {
    if (checkFilter(stripFilter, {0, 0})) {
        render1c(shader, {offset, 0, 0}, slice[1][1].a);
    }
    for (int i = 1; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            std::array<int, 3> pos = {0, 0, 0};
            pos[i] += j * -2 + 1;
            if (checkFilter(stripFilter, {pos[1], pos[2]})) {
                Piece piece = slice[pos[1] + 1][pos[2] + 1];
                CellLocation orientation = (CellLocation)(i * 2 + j + 2);
                render2c(shader, {(float)pos[0] + offset, (float)pos[1], (float)pos[2]}, {piece.a, piece.b}, orientation);
            }
        }
    }

    for (int j = 0; j < 2; j++) {
        for (int k = 0; k < 2; k++) {
            std::array<int, 3> pos = {0, 0, 0};
            pos[1] = k * -2 + 1;
            pos[2] = j * -2 + 1;
            if (checkFilter(stripFilter, {pos[1], pos[2]})) {
                Piece piece = slice[pos[1] + 1][pos[2] + 1];
                render3c(shader, {(float)pos[0] + offset, (float)pos[1], (float)pos[2]}, {piece.a, piece.b, piece.c});
            }
        }
    }
}

void PuzzleRenderer::renderMiddleSlice(Shader *shader, bool addOffsetX, float offsetYZ) {
    float offset = (addOffsetX ? -0.5 * puzzle->outerSlicePos : 0.0f) + 2 * puzzle->middleSlicePos;
    render1c(shader, {offset, 2 + offsetYZ, 0}, puzzle->topCell.a);
    render1c(shader, {offset, -2 - offsetYZ, 0}, puzzle->bottomCell.a);
    render1c(shader, {offset, 0, 2 + offsetYZ}, puzzle->frontCell[1].a);
    render1c(shader, {offset, 0, -2 - offsetYZ}, puzzle->backCell[1].a);

    if (puzzle->middleSliceDir == FRONT) {
        render2c(shader, {offset, 1, 2 + offsetYZ}, {puzzle->frontCell[2].a, puzzle->frontCell[2].b}, UP);
        render2c(shader, {offset, -1, 2 + offsetYZ}, {puzzle->frontCell[0].a, puzzle->frontCell[0].b}, DOWN);
        render2c(shader, {offset, 1, -2 - offsetYZ}, {puzzle->backCell[2].a, puzzle->backCell[2].b}, UP);
        render2c(shader, {offset, -1, -2 - offsetYZ}, {puzzle->backCell[0].a, puzzle->backCell[0].b}, DOWN);
    } else {
        render2c(shader, {offset, 2 + offsetYZ, 1}, {puzzle->frontCell[2].a, puzzle->frontCell[2].b}, BACK);
        render2c(shader, {offset, -2 - offsetYZ, 1}, {puzzle->frontCell[0].a, puzzle->frontCell[0].b}, BACK);
        render2c(shader, {offset, 2 + offsetYZ, -1}, {puzzle->backCell[2].a, puzzle->backCell[2].b}, FRONT);
        render2c(shader, {offset, -2 - offsetYZ, -1}, {puzzle->backCell[0].a, puzzle->backCell[0].b}, FRONT);
    }
}

void PuzzleRenderer::renderPuzzle(Shader *shader) {
    if (pendingMoves.size() == 0) {
        renderNoAnimation(shader);
    } else if (pendingMoves.front().type == TURN) {
        MoveEntry move = pendingMoves.front();
        switch (move.cell) {
            case LEFT: renderLeftAnimation(shader, move.direction); break;
            case RIGHT: renderRightAnimation(shader, move.direction); break;
            case IN: renderInnerAnimation(shader, move.direction); break;
            case OUT: renderOuterAnimation(shader, move.direction); break;
            default:
                // todo
                break;
        }
    } else if (pendingMoves.front().type == ROTATE) {
        renderRotateAnimation(shader, pendingMoves.front().direction);
    } else if (pendingMoves.front().type == GYRO) {
        MoveEntry move = pendingMoves.front();
        switch (move.cell) {
            case LEFT:
            case RIGHT:
                renderGyroXAnimation(shader, move.cell);
                break;
            case UP:
            case DOWN:
                renderGyroYAnimation(shader, move.cell);
                break;
            case FRONT:
            case BACK:
                renderGyroZAnimation(shader, move.cell);
                break;
            default:
                return;
        }
    } else if (pendingMoves.front().type == GYRO_OUTER) {
        renderOuterGyroAnimation(shader, pendingMoves.front().location);
    } else if (pendingMoves.front().type == GYRO_MIDDLE) {
        renderMiddleGyroAnimation(shader, pendingMoves.front().location);
    }
}

void PuzzleRenderer::renderNoAnimation(Shader *shader) {
    float offset = puzzle->outerSlicePos * -0.5f;
    mat4x4_identity(model);
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset);
    renderCell(shader, puzzle->rightCell, 2.0f + offset);
    renderSlice(shader, puzzle->innerSlice, offset);
    renderMiddleSlice(shader, true);
}

void PuzzleRenderer::renderLeftAnimation(Shader *shader, RotateDirection direction) {
    if (puzzle->outerSlicePos == 1) {
        float offset = 2 * animationProgress * (animationProgress - 1.0f);
        std::array<float, 3> axis = {0, 0, 0};
        axis[(int)direction / 2] = -1 + (int)direction % 2 * 2;

        mat4x4_translate(model, -2.5f + offset, 0, 0);
        mat4x4_rotate(model, model, axis[0], axis[1], axis[2], M_PI_2 * animationProgress);
        renderCell(shader, puzzle->leftCell, 0.0f);

        mat4x4_identity(model);
        if (puzzle->middleSlicePos == -1) renderMiddleSlice(shader, true, -offset);

        mat4x4_translate(model, -offset, 0, 0);
        renderCell(shader, puzzle->rightCell, 1.5f);
        renderSlice(shader, puzzle->innerSlice, -0.5f);
        renderSlice(shader, puzzle->outerSlice, 3.5f);
        if (puzzle->middleSlicePos >= 0) renderMiddleSlice(shader, true);
    } else {
        float offset = 4 * animationProgress * (animationProgress - 1.0f);
        std::array<float, 3> axis = {0, 0, 0};
        axis[(int)direction / 2] = -1 + (int)direction % 2 * 2;

        mat4x4_translate(model, -1.5f, 0, 0);
        mat4x4_rotate(model, model, axis[0], axis[1], axis[2], M_PI_2 * animationProgress);
        renderCell(shader, puzzle->leftCell, 0.0f);

        mat4x4_identity(model);
        if (puzzle->middleSlicePos == -1) renderMiddleSlice(shader, true, -offset);

        mat4x4_translate(model, offset, 0, 0);
        renderSlice(shader, puzzle->outerSlice, -3.5f);
        if (puzzle->middleSlicePos == -2) renderMiddleSlice(shader, true);

        mat4x4_translate(model, -offset, 0, 0);
        renderCell(shader, puzzle->rightCell, 2.5f);
        renderSlice(shader, puzzle->innerSlice, 0.5f);
        if (puzzle->middleSlicePos >= 0) renderMiddleSlice(shader, true);
    }
}

void PuzzleRenderer::renderRightAnimation(Shader *shader, RotateDirection direction) {
    if (puzzle->outerSlicePos == -1) {
        float offset = 2 * animationProgress * (animationProgress - 1.0f);
        std::array<float, 3> axis = {0, 0, 0};
        axis[(int)direction / 2] = -1 + (int)direction % 2 * 2;

        mat4x4_translate(model, 2.5f - offset, 0, 0);
        mat4x4_rotate(model, model, axis[0], axis[1], axis[2], M_PI_2 * animationProgress);
        renderCell(shader, puzzle->rightCell, 0.0f);

        mat4x4_identity(model);
        if (puzzle->middleSlicePos == 1) renderMiddleSlice(shader, true, -offset);

        mat4x4_translate(model, offset, 0, 0);
        renderCell(shader, puzzle->leftCell, -1.5f);
        renderSlice(shader, puzzle->innerSlice, 0.5f);
        renderSlice(shader, puzzle->outerSlice, -3.5f);
        if (puzzle->middleSlicePos <= 0) renderMiddleSlice(shader, true, -offset);
    } else {
        float offset = 4 * animationProgress * (animationProgress - 1.0f);
        std::array<float, 3> axis = {0, 0, 0};
        axis[(int)direction / 2] = -1 + (int)direction % 2 * 2;

        mat4x4_translate(model, 1.5f, 0, 0);
        mat4x4_rotate(model, model, axis[0], axis[1], axis[2], M_PI_2 * animationProgress);
        renderCell(shader, puzzle->rightCell, 0.0f);

        mat4x4_identity(model);
        if (puzzle->middleSlicePos == 1) renderMiddleSlice(shader, true, -offset);

        mat4x4_translate(model, -offset, 0, 0);
        renderSlice(shader, puzzle->outerSlice, 3.5f);
        if (puzzle->middleSlicePos == 2) renderMiddleSlice(shader, true);

        mat4x4_translate(model, offset, 0, 0);
        renderCell(shader, puzzle->leftCell, -2.5f);
        renderSlice(shader, puzzle->innerSlice, -0.5f);
        if (puzzle->middleSlicePos <= 0) renderMiddleSlice(shader, true);
    }
}

void PuzzleRenderer::renderInnerAnimation(Shader *shader, RotateDirection direction) {
    float offset = puzzle->outerSlicePos * -0.5f;
    mat4x4_identity(model);
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {0, -1, -1});
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {1, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {1, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {2, -1, -1});

    float offsetYZ = -4 * animationProgress * (animationProgress - 1.0f);
    if (puzzle->middleSlicePos == 0) {
        renderMiddleSlice(shader, true, offsetYZ);
    } else {
        renderMiddleSlice(shader, true, 0.0f);
    }

    int parity = (int)direction % 2 * 2 - 1;
    mat4x4_rotate(model, model, 1, 0, 0, M_PI_2 * animationProgress * parity);
    renderSlice(shader, puzzle->innerSlice, offset);
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {2, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {0, -1, -1});
}

void PuzzleRenderer::renderOuterAnimation(Shader *shader, RotateDirection direction) {
    float offset = puzzle->outerSlicePos * -0.5f;
    mat4x4_identity(model);
    renderSlice(shader, puzzle->innerSlice, offset);
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {1, -1, -1});
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {2, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {0, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {1, -1, -1});

    float offsetYZ = -4 * animationProgress * (animationProgress - 1.0f);
    if (puzzle->middleSlicePos == 2 * puzzle->outerSlicePos) {
        renderMiddleSlice(shader, true, offsetYZ);
    } else {
        renderMiddleSlice(shader, true, 0.0f);
    }

    int parity = (int)direction % 2 * 2 - 1;
    mat4x4_rotate(model, model, 1, 0, 0, M_PI_2 * animationProgress * parity);
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset, {0, -1, -1});
    renderCell(shader, puzzle->rightCell, 2.0f + offset, {2, -1, -1});
}

void PuzzleRenderer::renderRotateAnimation(Shader *shader, RotateDirection direction) {
    int parity = (int)direction * 2 - 1;
    mat4x4_identity(model);
    mat4x4_rotate(model, model, 1, 0, 0, parity * M_PI_2 * animationProgress);

    float offset = puzzle->outerSlicePos * -0.5f;
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset);
    renderCell(shader, puzzle->rightCell, 2.0f + offset);
    renderSlice(shader, puzzle->innerSlice, offset);
    renderMiddleSlice(shader, true);
}

void PuzzleRenderer::renderGyroXAnimation(Shader *shader, CellLocation cell) {
    int mainDir = (int)cell % 2 * -2 + 1;
    int moveSlice = (1 - mainDir * puzzle->outerSlicePos) / 2;
    std::array<CellData*, 2> cells = {&puzzle->leftCell, &puzzle->rightCell};
    int left = (int)cell % 2;
    int right = 1 - left;
    if (animationProgress < 2.0f) {
        int middleMove = -1 + 3 * moveSlice;
        float mainOffsetX, mainOffsetY, slicePosX, slicePosY;
        if (animationProgress < 0.5f) {
            slicePosX = -0.5f * puzzle->outerSlicePos - 2 * mainDir;
            slicePosY = -16 * animationProgress * (animationProgress - 1.0f);
            mainOffsetX = -0.5f * puzzle->outerSlicePos;
            mainOffsetY = 2 * animationProgress * (animationProgress - 1.0f);
        } else if (animationProgress < 1.5f) {
            slicePosX = -0.5f * puzzle->outerSlicePos + mainDir * (smoothstep(animationProgress - 0.5f) * 6 - 2);
            slicePosY = 4.0f;
            mainOffsetX = -0.5f * puzzle->outerSlicePos - mainDir * smoothstep(animationProgress - 0.5f) * 2;
            mainOffsetY = -0.5f;
        } else {
            slicePosX = -0.5f * puzzle->outerSlicePos + 4 * mainDir;
            slicePosY = -16 * (animationProgress - 1.0f) * (animationProgress - 2.0f);
            mainOffsetX = -0.5f * puzzle->outerSlicePos - 2 * mainDir;
            mainOffsetY = 2 * (animationProgress - 1.0f) * (animationProgress - 2.0f);
        }
        mat4x4_translate(model, slicePosX, slicePosY, 0);
        renderCell(shader, *cells[left], 0.0f, {1 - mainDir, -1, -1});
        if (mainDir * puzzle->outerSlicePos == 1) {
            renderCell(shader, *cells[left], 0.0f, {1, -1, -1});
        } else {
            renderSlice(shader, puzzle->outerSlice, 2.0f * puzzle->outerSlicePos);
        }
        // Undo X offset made by renderMiddleSlice
        float reset = -2 * puzzle->middleSlicePos;
        mat4x4_translate_in_place(model, reset - mainDir + puzzle->outerSlicePos, 0, 0);
        if (puzzle->middleSlicePos == middleMove * puzzle->outerSlicePos) renderMiddleSlice(shader, false);

        mat4x4_translate(model, mainOffsetX, mainOffsetY, 0);
        if (mainDir * puzzle->outerSlicePos == 1) {
            renderSlice(shader, puzzle->outerSlice, 4.0f * puzzle->outerSlicePos);
        } else {
            renderCell(shader, *cells[left], 2.0f * puzzle->outerSlicePos, {1, -1, -1});
        }
        renderCell(shader, *cells[left], -2.0f * mainDir, {1 + mainDir, -1, -1});
        renderCell(shader, *cells[right], 2.0f * mainDir);
        renderSlice(shader, puzzle->innerSlice, 0.0f);
        if (puzzle->middleSlicePos != middleMove * puzzle->outerSlicePos) renderMiddleSlice(shader, false);
    } else {
        float offset = (cosf(M_PI * (animationProgress + 1.0f)) + 1) / 8;
        mat4x4_identity(model);

        std::array<SliceData*, 8> slices = {
            &(*cells[left])[2], &puzzle->innerSlice, &(*cells[right])[0], &(*cells[right])[1],
            &(*cells[right])[2], &puzzle->outerSlice, &(*cells[left])[0], &(*cells[left])[1]
        };
        if (left == 1) {
            std::swap(slices[1], slices[5]);
        }
        if (puzzle->outerSlicePos == -1) {
            std::rotate(slices.rbegin(), slices.rbegin() + 1, slices.rend());
        }
        for (int i = (puzzle->outerSlicePos + 1) / 2; i < 8; i += 2) {
            renderSlice(shader, *slices[i], i - 3.5 + offset * (-9 + 2 * i));
        }

        int newMiddlePos;
        if (puzzle->outerSlicePos == -1) {
            newMiddlePos = (puzzle->middleSlicePos - mainDir + 6) % 4 - 2;
        } else {
            newMiddlePos = (puzzle->middleSlicePos - mainDir + 5) % 4 - 1;
        }
        float newMiddleOffset = -2 * puzzle->middleSlicePos + newMiddlePos * 2 + offset * (4 * newMiddlePos - 2 - puzzle->outerSlicePos);
        float rotProgress = smoothstep((animationProgress - 2.0f) / 2);
        mat4x4_translate(model, newMiddleOffset, 0, 0);
        renderMiddleSlice(shader, true);

        for (int i = (1 - puzzle->outerSlicePos) / 2; i < 8; i += 2) {
            CellLocation direction = (CellLocation)(i / 2 % 2 + 2);
            mat4x4 baseModel;
            mat4x4_translate(baseModel, i - 3.5 + offset * (-9 + 2 * i), 0, 0);

            mat4x4_dup(model, baseModel);
            render2c(shader, {0, 0, 0}, {(*slices[i])[1][1].a, (*slices[i])[1][1].b}, direction);

            int flipRot = i / 2 % 2 * 2 - 1;
            for (int j = 0; j < 2; j++) {
                for (int k = -1; k < 2; k += 2) {
                    std::array<float, 2> pos = {0, 0};
                    pos[j] = k;
                    Piece piece = (*slices[i])[pos[0] + 1][pos[1] + 1];
                    pos[j] = k * (1 + offset * 4);
                    mat4x4_dup(model, baseModel);
                    mat4x4_translate_in_place(model, 0, pos[0], pos[1]);
                    mat4x4_rotate(model, model, flipRot * mainDir, 0, 0, M_PI_2 * rotProgress);
                    render3c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c});
                }
            }

            for (int j = -1; j < 2; j += 2) {
                for (int k = -1; k < 2; k += 2) {
                    std::array<float, 2> pos = {(float)j, (float)k};
                    Piece piece = (*slices[i])[pos[0] + 1][pos[1] + 1];
                    pos[0] *= (1 + offset * 4);
                    pos[1] *= (1 + offset * 4);
                    int orientation = 2 - 2 * j + (3 + flipRot * (k + 2)) / 2;
                    mat4x4_dup(model, baseModel);
                    mat4x4_translate_in_place(model, 0, pos[0], pos[1]);
                    mat4x4_rotate(model, model, 0, k * flipRot, 0, M_PI * rotProgress);
                    mat4x4_rotate(model, model, j * k, 0, 0, M_PI_2 * rotProgress);
                    render4c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c, piece.d}, orientation);
                }
            }
        }
    }
}

void PuzzleRenderer::renderGyroYAnimation(Shader *shader, CellLocation cell) {
    int direction = puzzle->outerSlicePos * ((int)cell % 2 * -2 + 1);
    std::array<CellData*, 2> cells = {&puzzle->leftCell, &puzzle->rightCell};
    int left = (1 - puzzle->outerSlicePos) / 2;
    int right = 1 - left;
    if (animationProgress < 1.0f) {
        float halfOffset = -2 * animationProgress * (animationProgress - 1.0f);

        mat4x4_translate(model, puzzle->outerSlicePos * -(halfOffset + 2.5f), 0, 0);
        mat4x4_rotate(model, model, 0, 0, -direction, M_PI_2 * animationProgress);
        renderCell(shader, *cells[left], 0);

        mat4x4_translate(model, puzzle->outerSlicePos * (halfOffset + 1.5f), 0, 0);
        mat4x4_rotate(model, model, 0, 0, direction, M_PI_2 * animationProgress);
        mat4x4_translate_in_place(model, puzzle->outerSlicePos * -1.5f, 0, 0);
        renderSlice(shader, puzzle->innerSlice, puzzle->outerSlicePos * -0.5f);
        renderMiddleSlice(shader, true);
        renderCell(shader, *cells[right], puzzle->outerSlicePos * 1.5f);
        renderSlice(shader, puzzle->outerSlice, puzzle->outerSlicePos * 3.5f);
    } else {
        mat4x4_translate(model, puzzle->outerSlicePos * -2.5f, 0, 0);
        mat4x4_rotate(model, model, 0, 0, -direction, M_PI_2);
        renderCell(shader, *cells[left], 0);

        mat4x4 base;
        mat4x4_translate(base, puzzle->outerSlicePos * 1.5f, 0, 0);
        mat4x4_rotate(base, base, 0, 0, direction, M_PI_2);
        mat4x4_translate_in_place(base, puzzle->outerSlicePos * -1.5f, 0, 0);

        mat4x4_dup(model, base);
        renderSlice(shader, puzzle->innerSlice, puzzle->outerSlicePos * -0.5f, {1, -1});
        renderMiddleSlice(shader, true);
        renderCell(shader, *cells[right], puzzle->outerSlicePos * 1.5f);
        renderSlice(shader, puzzle->outerSlice, puzzle->outerSlicePos * 3.5f, {1, -1});

        float stripYRotation = std::min(1.0f, std::max(0.0f, (animationProgress - 1.0f) * 2));
        float stripXOffset = std::min(1.0f, std::max(0.0f, (animationProgress - 1.5f) * 2));
        stripYRotation = -M_PI_2 * stripYRotation * puzzle->outerSlicePos;
        stripXOffset = (stripXOffset + 0.5f) * puzzle->outerSlicePos;
        int stripFilter = (animationProgress < 2.0f) ? -1 : 1;
        for (int i = -1; i < 2; i += 2) {
            mat4x4_dup(model, base);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * -0.5f + stripXOffset, i * 1.5f, 0);
            mat4x4_rotate(model, model, 0, 0, i, stripYRotation);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * -0.5f, -i * 1.5f, 0);
            renderSlice(shader, puzzle->innerSlice, 0, {1 + i, stripFilter});
            mat4x4_dup(model, base);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * 3.5f - stripXOffset, i * 1.5f, 0);
            mat4x4_rotate(model, model, 0, 0, i, -stripYRotation);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * 0.5f, -i * 1.5f, 0);
            renderSlice(shader, puzzle->outerSlice, 0, {1 + i, stripFilter});
        }

        if (animationProgress > 2.0f) {
            mat4x4_identity(model);
            for (int i = -1; i < 2; i += 2) {
                for (int j = -1; j < 2; j += 2) {
                    float offsetX = puzzle->outerSlicePos * 1.5f - 2 * direction * j;
                    float offsetY = puzzle->outerSlicePos * direction;
                    float offsetZ = i * (1 + sinf(M_PI * (animationProgress - 2.0f)));
                    float angle = M_PI_2 * smoothstep(animationProgress - 2.0f);

                    Piece piece = puzzle->outerSlice[j + 1][i + 1];
                    mat4x4_translate(model, offsetX, offsetY, offsetZ);
                    mat4x4_rotate(model, model, 0, -j, 0, angle);
                    render3c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c});

                    piece = puzzle->innerSlice[j + 1][i + 1];
                    mat4x4_translate(model, offsetX, -offsetY, offsetZ);
                    mat4x4_rotate(model, model, 0, -j, 0, angle);
                    render3c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c});
                }
            }
        }
    }
}

void PuzzleRenderer::renderGyroZAnimation(Shader *shader, CellLocation cell) {
    int direction = puzzle->outerSlicePos * ((int)cell % 2 * 2 - 1);
    std::array<CellData*, 2> cells = {&puzzle->leftCell, &puzzle->rightCell};
    int left = (1 - puzzle->outerSlicePos) / 2;
    int right = 1 - left;
    if (animationProgress < 1.0f) {
        float halfOffset = -2 * animationProgress * (animationProgress - 1.0f);

        mat4x4_translate(model, puzzle->outerSlicePos * -(halfOffset + 2.5f), 0, 0);
        mat4x4_rotate(model, model, 0, -direction, 0, M_PI_2 * animationProgress);
        renderCell(shader, *cells[left], 0);

        mat4x4_translate(model, puzzle->outerSlicePos * (halfOffset + 1.5f), 0, 0);
        mat4x4_rotate(model, model, 0, direction, 0, M_PI_2 * animationProgress);
        mat4x4_translate_in_place(model, puzzle->outerSlicePos * -1.5f, 0, 0);
        renderSlice(shader, puzzle->innerSlice, puzzle->outerSlicePos * -0.5f);
        renderMiddleSlice(shader, true);
        renderCell(shader, *cells[right], puzzle->outerSlicePos * 1.5f);
        renderSlice(shader, puzzle->outerSlice, puzzle->outerSlicePos * 3.5f);
    } else {
        mat4x4_translate(model, puzzle->outerSlicePos * -2.5f, 0, 0);
        mat4x4_rotate(model, model, 0, -direction, 0, M_PI_2);
        renderCell(shader, *cells[left], 0);

        mat4x4 base;
        mat4x4_translate(base, puzzle->outerSlicePos * 1.5f, 0, 0);
        mat4x4_rotate(base, base, 0, direction, 0, M_PI_2);
        mat4x4_translate_in_place(base, puzzle->outerSlicePos * -1.5f, 0, 0);

        mat4x4_dup(model, base);
        renderSlice(shader, puzzle->innerSlice, puzzle->outerSlicePos * -0.5f, {-1, 1});
        renderMiddleSlice(shader, true);
        renderCell(shader, *cells[right], puzzle->outerSlicePos * 1.5f);
        renderSlice(shader, puzzle->outerSlice, puzzle->outerSlicePos * 3.5f, {-1, 1});

        float stripZRotation = std::min(1.0f, std::max(0.0f, (animationProgress - 1.0f) * 2));
        float stripXOffset = std::min(1.0f, std::max(0.0f, (animationProgress - 1.5f) * 2));
        stripZRotation = M_PI_2 * stripZRotation * puzzle->outerSlicePos;
        stripXOffset = (stripXOffset + 0.5f) * puzzle->outerSlicePos;
        int stripFilter = (animationProgress < 2.0f) ? -1 : 1;
        for (int i = -1; i < 2; i += 2) {
            mat4x4_dup(model, base);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * -0.5f + stripXOffset, 0, i * 1.5f);
            mat4x4_rotate(model, model, 0, i, 0, stripZRotation);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * -0.5f, 0, -i * 1.5f);
            renderSlice(shader, puzzle->innerSlice, 0, {stripFilter, 1 + i});
            mat4x4_dup(model, base);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * 3.5f - stripXOffset, 0, i * 1.5f);
            mat4x4_rotate(model, model, 0, i, 0, -stripZRotation);
            mat4x4_translate_in_place(model, puzzle->outerSlicePos * 0.5f, 0, -i * 1.5f);
            renderSlice(shader, puzzle->outerSlice, 0, {stripFilter, 1 + i});
        }

        if (animationProgress > 2.0f) {
            mat4x4_identity(model);
            for (int i = -1; i < 2; i += 2) {
                for (int j = -1; j < 2; j += 2) {
                    float offsetX = puzzle->outerSlicePos * 1.5f + 2 * direction * j;
                    float offsetY = i * (1 + sinf(M_PI * (animationProgress - 2.0f)));
                    float offsetZ = puzzle->outerSlicePos * direction;
                    float angle = M_PI_2 * smoothstep(animationProgress - 2.0f);

                    Piece piece = puzzle->innerSlice[i + 1][j + 1];
                    mat4x4_translate(model, offsetX, offsetY, offsetZ);
                    mat4x4_rotate(model, model, 0, 0, -j, angle);
                    render3c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c});

                    piece = puzzle->outerSlice[i + 1][j + 1];
                    mat4x4_translate(model, offsetX, offsetY, -offsetZ);
                    mat4x4_rotate(model, model, 0, 0, -j, angle);
                    render3c(shader, {0, 0, 0}, {piece.a, piece.b, piece.c});
                }
            }
        }
    }
}

void PuzzleRenderer::renderOuterGyroAnimation(Shader *shader, int location) {
    float mainOffsetX, mainOffsetY, slicePosX, slicePosY;
    if (animationProgress < 0.5f) {
        slicePosX = puzzle->outerSlicePos * 3.5f;
        slicePosY = -16 * animationProgress * (animationProgress - 1.0f);
        mainOffsetX = puzzle->outerSlicePos * -0.5f;
        mainOffsetY = 2 * animationProgress * (animationProgress - 1.0f);
    } else if (animationProgress < 1.5f) {
        slicePosX = (3.5 - smoothstep(animationProgress - 0.5f) * 7) * puzzle->outerSlicePos;
        slicePosY = 4.0f;
        mainOffsetX = (-0.5 + smoothstep(animationProgress - 0.5f)) * puzzle->outerSlicePos;
        mainOffsetY = -0.5f;
    } else {
        slicePosX = puzzle->outerSlicePos * -3.5f;
        slicePosY = -16 * (animationProgress - 1.0f) * (animationProgress - 2.0f);
        mainOffsetX = puzzle->outerSlicePos * 0.5f;
        mainOffsetY = 2 * (animationProgress - 1.0f) * (animationProgress - 2.0f);
    }
    mat4x4_translate(model, slicePosX, slicePosY, 0);
    renderSlice(shader, puzzle->outerSlice, 0.0f);
    // Undo X offset made by renderMiddleSlice
    mat4x4_translate_in_place(model, -2 * puzzle->middleSlicePos, 0, 0);
    if (puzzle->outerSlicePos * 2 == puzzle->middleSlicePos) renderMiddleSlice(shader, false);

    mat4x4_translate(model, mainOffsetX, mainOffsetY, 0);
    renderCell(shader, puzzle->leftCell, -2.0f);
    renderCell(shader, puzzle->rightCell, 2.0f);
    renderSlice(shader, puzzle->innerSlice, 0.0f);
    if (puzzle->outerSlicePos * 2 != puzzle->middleSlicePos) renderMiddleSlice(shader, false);
}

void PuzzleRenderer::renderMiddleGyroAnimation(Shader *shader, int direction) {
    if (direction == 0) {
        renderMiddleGyroDirAnimation(shader);
    } else {
        renderMiddleGyroPosAnimation(shader, direction);
    }
}

void PuzzleRenderer::renderMiddleGyroDirAnimation(Shader *shader) {
    float offset = puzzle->outerSlicePos * -0.5f;
    mat4x4_identity(model);
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset);
    renderCell(shader, puzzle->rightCell, 2.0f + offset);
    renderSlice(shader, puzzle->innerSlice, offset);

    offset += 2 * puzzle->middleSlicePos;
    render1c(shader, {offset, 2, 0}, puzzle->topCell.a);
    render1c(shader, {offset, -2, 0}, puzzle->bottomCell.a);
    render1c(shader, {offset, 0, 2}, puzzle->frontCell[1].a);
    render1c(shader, {offset, 0, -2}, puzzle->backCell[1].a);

    float parity = (puzzle->middleSliceDir == UP) ? 1.0f : -1.0f;
    std::array<CellLocation, 8> targets = {DOWN, DOWN, UP, UP, FRONT, BACK, FRONT, BACK};
    if (puzzle->middleSliceDir == UP) {
        std::rotate(targets.begin(), targets.begin() + 4, targets.end());
    }
    if (animationProgress < 0.5f) {
        std::array<std::array<Piece, 3>, 3> cells;
        cells[0] = puzzle->backCell;
        cells[2] = puzzle->frontCell;

        for (int i = -1; i < 2; i += 2) {
            for (int j = -1; j < 2; j += 2) {
                CellLocation target = targets[i + 1 + (j + 1) / 2];
                mat4x4_translate(model, 0, 1.5 * i, 1.5 * j);
                mat4x4_rotate(model, model, i * j, 0, 0, parity * M_PI * animationProgress);
                mat4x4_translate_in_place(model, 0, i * parity * 0.5, j * -parity * 0.5);
                render2c(shader, {offset, 0, 0}, {cells[j + 1][i + 1].a, cells[j + 1][i + 1].b}, target);
            }
        }
    } else {
        float glide = 1 - 4 * animationProgress * (animationProgress - 1.0f);
        float glideY = (puzzle->middleSliceDir == UP) ? glide : 2;
        float glideZ = (puzzle->middleSliceDir != UP) ? glide : 2;
        render2c(shader, {offset, -glideY, -glideZ}, {puzzle->backCell[0].a, puzzle->backCell[0].b}, targets[4]);
        render2c(shader, {offset, -glideY, glideZ}, {puzzle->frontCell[0].a, puzzle->frontCell[0].b}, targets[5]);
        render2c(shader, {offset, glideY, -glideZ}, {puzzle->backCell[2].a, puzzle->backCell[2].b}, targets[6]);
        render2c(shader, {offset, glideY, glideZ}, {puzzle->frontCell[2].a, puzzle->frontCell[2].b}, targets[7]);
    }
}

void PuzzleRenderer::renderMiddleGyroPosAnimation(Shader *shader, int direction) {
    float offset = puzzle->outerSlicePos * -0.5f;
    mat4x4_identity(model);
    renderSlice(shader, puzzle->outerSlice, 3.5f * puzzle->outerSlicePos);
    renderCell(shader, puzzle->leftCell, -2.0f + offset);
    renderCell(shader, puzzle->rightCell, 2.0f + offset);
    renderSlice(shader, puzzle->innerSlice, offset);

    offset += 2 * puzzle->middleSlicePos;
    float arcX = direction + -direction * cos(M_PI * animationProgress);
    float arcY = sin(M_PI * animationProgress);

    mat4x4_translate(model, offset + arcX, 2 + arcY, 0);
    mat4x4_rotate(model, model, 0, 0, -1, direction * M_PI * animationProgress);
    render1c(shader, {0, 0, 0}, puzzle->topCell.a);
    if (puzzle->middleSliceDir == UP) {
        render2c(shader, {0, 0, 1}, {puzzle->frontCell[2].a, puzzle->frontCell[2].b}, BACK);
        render2c(shader, {0, 0, -1}, {puzzle->backCell[2].a, puzzle->backCell[2].b}, FRONT);
    }

    mat4x4_translate(model, offset + arcX, -2 - arcY, 0);
    mat4x4_rotate(model, model, 0, 0, -1, -direction * M_PI * animationProgress);
    render1c(shader, {0, 0, 0}, puzzle->bottomCell.a);
    if (puzzle->middleSliceDir == UP) {
        render2c(shader, {0, 0, 1}, {puzzle->frontCell[0].a, puzzle->frontCell[0].b}, BACK);
        render2c(shader, {0, 0, -1}, {puzzle->backCell[0].a, puzzle->backCell[0].b}, FRONT);
    }

    mat4x4_translate(model, offset + arcX, 0, 2 + arcY);
    mat4x4_rotate(model, model, 0, 1, 0, direction * M_PI * animationProgress);
    render1c(shader, {0, 0, 0}, puzzle->frontCell[1].a);
    if (puzzle->middleSliceDir == FRONT) {
        render2c(shader, {0, 1, 0}, {puzzle->frontCell[2].a, puzzle->frontCell[2].b}, UP);
        render2c(shader, {0, -1, 0}, {puzzle->frontCell[0].a, puzzle->frontCell[0].b}, DOWN);
    }

    mat4x4_translate(model, offset + arcX, 0, -2 - arcY);
    mat4x4_rotate(model, model, 0, 1, 0, -direction * M_PI * animationProgress);
    render1c(shader, {0, 0, 0}, puzzle->backCell[1].a);
    if (puzzle->middleSliceDir == FRONT) {
        render2c(shader, {0, 1, 0}, {puzzle->backCell[2].a, puzzle->backCell[2].b}, UP);
        render2c(shader, {0, -1, 0}, {puzzle->backCell[0].a, puzzle->backCell[0].b}, DOWN);
    }
}

void PuzzleRenderer::updateAnimations(GLFWwindow *window, double dt) {
    if (pendingMoves.size() == 0) {
        animating = false;
    }
    if (animating) {
        animationProgress += dt * animationSpeed;
        if (animationProgress > pendingMoves.front().animLength) {
            MoveEntry entry = pendingMoves.front();
            pendingMoves.pop();

            switch (entry.type) {
                case TURN: puzzle->rotateCell(entry.cell, entry.direction); break;
                case ROTATE: puzzle->rotatePuzzle(entry.direction); break;
                case GYRO: puzzle->gyroCell(entry.cell); break;
                case GYRO_OUTER: puzzle->gyroOuterSlice(); break;
                case GYRO_MIDDLE: puzzle->gyroMiddleSlice(entry.location); break;
            }

            animationProgress = 0.0f;
        }
    } else {
        if (checkMiddleGyro(window)) return;
        if (checkDirectionalMove(window)) return;

        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            // gyro outer layer
            MoveEntry entry;
            entry.type = GYRO_OUTER;
            entry.animLength = 2.0f;
            entry.location = -1 * puzzle->outerSlicePos;
            pendingMoves.push(entry);
            animating = true;
        }
    }
}

bool PuzzleRenderer::checkMiddleGyro(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_M) || glfwGetKey(window, GLFW_KEY_PERIOD)) {
        int direction = (glfwGetKey(window, GLFW_KEY_M)) ? -1 : 1;
        if (puzzle->canGyroMiddle(direction)) {
            MoveEntry entry;
            entry.type = GYRO_MIDDLE;
            entry.animLength = 1.0f;
            entry.location = direction;
            pendingMoves.push(entry);
            animating = true;
            return true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_COMMA)) {
        MoveEntry entry;
        entry.type = GYRO_MIDDLE;
        entry.animLength = 1.0f;
        entry.location = 0;
        pendingMoves.push(entry);
        animating = true;
        return true;
    }
    return false;
}

bool PuzzleRenderer::checkDirectionalMove(GLFWwindow* window) {
    int cellKeys[] = {GLFW_KEY_D, GLFW_KEY_V, GLFW_KEY_F, GLFW_KEY_W,
                      GLFW_KEY_E, GLFW_KEY_C, GLFW_KEY_S, GLFW_KEY_R};
    bool foundCell = false;
    CellLocation cell = (CellLocation)-1;
    for (int i = 0; i < 8; i++) {
        if (glfwGetKey(window, cellKeys[i])) {
            foundCell = true;
            cell = (CellLocation)i;
        }
    }
    int directionKeys[] = {GLFW_KEY_I, GLFW_KEY_K, GLFW_KEY_J, GLFW_KEY_L, GLFW_KEY_O, GLFW_KEY_U};
    bool foundDirection = false;
    RotateDirection direction = (RotateDirection)-1;
    for (int i = 0; i < 6; i++) {
        if (glfwGetKey(window, directionKeys[i])) {
            // Move outer layer
            foundDirection = true;
            direction = (RotateDirection)i;
        }
    }
    if (foundCell) {
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            startGyro(cell);
            animating = true;
            return true;
        }

        if (foundDirection) {
            if (puzzle->canRotateCell(cell, direction)) {
                MoveEntry entry;
                entry.type = TURN;
                entry.animLength = 1.0f;
                entry.cell = cell;
                entry.direction = direction;
                pendingMoves.push(entry);
                animating = true;
                return true;
            }
        }
    } else if (foundDirection && (direction == YZ || direction == ZY)) {
        // whole puzzle rotation
        MoveEntry entry;
        entry.type = ROTATE;
        entry.animLength = 1.0f;
        entry.direction = direction;
        pendingMoves.push(entry);
        animating = true;
        return true;
    }
    return false;
}

void PuzzleRenderer::startGyro(CellLocation cell) {
    MoveEntry entry;
    int direction = 0;
    switch (cell) {
        case LEFT:
        case RIGHT:
            entry.type = GYRO;
            entry.animLength = 4.0f;
            entry.cell = cell;
            pendingMoves.push(entry);
            break;
        case UP:
        case DOWN:
            if (puzzle->middleSliceDir == FRONT) {
                entry.type = GYRO_MIDDLE;
                entry.animLength = 1.0f;
                entry.location = 0;
                pendingMoves.push(entry);
            }

            if (puzzle->middleSlicePos == 0) {
                direction = puzzle->outerSlicePos;
            } else if (puzzle->middleSlicePos == 2 * puzzle->outerSlicePos) {
                direction = -puzzle->outerSlicePos;
            } else if (puzzle->middleSlicePos == -puzzle->outerSlicePos) {
                entry.type = GYRO_OUTER;
                entry.animLength = 2.0f;
                entry.location = -1 * puzzle->outerSlicePos;
                pendingMoves.push(entry);
                direction = 0;
            } else if (puzzle->middleSlicePos == puzzle->outerSlicePos) {
                direction = 0;
            }

            if (direction != 0) {
                entry.type = GYRO_MIDDLE;
                entry.animLength = 1.0f;
                entry.location = direction;
                pendingMoves.push(entry);
            }

            entry.type = GYRO;
            entry.animLength = 3.0f;
            entry.cell = cell;
            pendingMoves.push(entry);
            break;
        case FRONT:
        case BACK:
            if (puzzle->middleSliceDir == UP) {
                entry.type = GYRO_MIDDLE;
                entry.animLength = 1.0f;
                entry.location = 0;
                pendingMoves.push(entry);
            }

            if (puzzle->middleSlicePos == 0) {
                direction = puzzle->outerSlicePos;
            } else if (puzzle->middleSlicePos == 2 * puzzle->outerSlicePos) {
                direction = -puzzle->outerSlicePos;
            } else if (puzzle->middleSlicePos == -puzzle->outerSlicePos) {
                entry.type = GYRO_OUTER;
                entry.animLength = 2.0f;
                entry.location = -1 * puzzle->outerSlicePos;
                pendingMoves.push(entry);
                direction = 0;
            } else if (puzzle->middleSlicePos == puzzle->outerSlicePos) {
                direction = 0;
            }

            if (direction != 0) {
                entry.type = GYRO_MIDDLE;
                entry.animLength = 1.0f;
                entry.location = direction;
                pendingMoves.push(entry);
            }

            entry.type = GYRO;
            entry.animLength = 3.0f;
            entry.cell = cell;
            pendingMoves.push(entry);
            break;
        case IN:
        case OUT:
            return;
    }
}
