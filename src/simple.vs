layout(location = 0) in vec3 vsPos;

out vec3 fsPos;
out vec3 fsResult;

uniform mat4 uMvp;
uniform mat4 uView;

void main() {
    fsPos = vsPos;

    gl_Position = uMvp * vec4(vsPos, 1.0);
}
