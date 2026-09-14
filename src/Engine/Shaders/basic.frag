#version 330 core

uniform vec4 uColor; // текущий цвет
uniform int uPointMode; // 0 - ничего, 1 - круг, 2 - квадрат
uniform float uPointSoft; // мягкость края точки

uniform vec3 uLightColor;
uniform vec3 uLightPosition;
uniform float uLightIntensity;
uniform vec3 uViewPosition;

uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform float uSpecularStrength;
uniform float uShininess;

uniform int uLightingEnabled;

in vec3 vFragPos;
in vec3 vNormal;

out vec4 FragColor;

void main() {
    if (uPointMode == 1) {
        vec2 p = gl_PointCoord - vec2(0.5);
        float r = length(p);
        float edge0 = 0.5 - uPointSoft;
        float edge1 = 0.5;
        float alpha = 1.0 - smoothstep(edge0, edge1, r);

        if (alpha <= 0.0) discard;

        FragColor = vec4(uColor.rgb, uColor.a * alpha);
        return;
    }

    else if (uPointMode == 2) {
        vec2 p = abs(gl_PointCoord - vec2(0.5));
        float d = max(p.x, p.y);
        float edge0 = 0.5 - uPointSoft;
        float edge1 = 0.5;
        float alpha = 1.0 - smoothstep(edge0, edge1, d);

        if (alpha <= 0.0) discard;

        FragColor = vec4(uColor.rgb, uColor.a * alpha);
        return;
    }

    if (uLightingEnabled == 0) {
        FragColor = uColor;
        return;
    }

    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vFragPos);

    vec3 ambient =
        uAmbientStrength *
        uLightColor *
        uLightIntensity;

    float diffuseFactor =
        max(dot(normal, lightDirection), 0.0);

    vec3 diffuse =
        uDiffuseStrength *
        diffuseFactor *
        uLightColor *
        uLightIntensity;

    vec3 viewDirection =
        normalize(uViewPosition - vFragPos);

    vec3 reflectDirection =
        reflect(-lightDirection, normal);

    float specularFactor = pow(
        max(dot(viewDirection, reflectDirection), 0.0),
        uShininess
    );

    vec3 specular =
        uSpecularStrength *
        specularFactor *
        uLightColor *
        uLightIntensity;

    vec3 lighting =
        ambient +
        diffuse +
        specular;

    vec3 result =
        lighting *
        uColor.rgb;

    FragColor = vec4(result, uColor.a);
}