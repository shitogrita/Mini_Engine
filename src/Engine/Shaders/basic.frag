#version 330 core

uniform vec4 uColor; // текущий цвет
uniform float uDashFill; // ширина штриха
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

/*
 * Diffuse texture материала.
 *
 * uHasDiffuseTexture = 0:
 * используется только uColor.
 *
 * uHasDiffuseTexture = 1:
 * цвет текстуры умножается на uColor.
 */
uniform sampler2D uDiffuseTexture;
uniform int uHasDiffuseTexture;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;


void main() {
    /*
     * Режим круглой точки.
     */
    if (uPointMode == 1) {
        vec2 p = gl_PointCoord - vec2(0.5);
        float r = length(p);

        float edge0 = 0.5 - uPointSoft;
        float edge1 = 0.5;

        float alpha =
            1.0 -
            smoothstep(
                edge0,
                edge1,
                r
            );

        if (alpha <= 0.0) {
            discard;
        }

        FragColor =
            vec4(
                uColor.rgb,
                uColor.a * alpha
            );

        return;
    }


    /*
     * Режим квадратной точки.
     */
    else if (uPointMode == 2) {
        vec2 p =
            abs(
                gl_PointCoord -
                vec2(0.5)
            );

        float d =
            max(
                p.x,
                p.y
            );

        float edge0 =
            0.5 -
            uPointSoft;

        float edge1 =
            0.5;

        float alpha =
            1.0 -
            smoothstep(
                edge0,
                edge1,
                d
            );

        if (alpha <= 0.0) {
            discard;
        }

        FragColor =
            vec4(
                uColor.rgb,
                uColor.a * alpha
            );

        return;
    }


    /*
     * Базовый цвет поверхности.
     *
     * Если у Material нет текстуры,
     * используется обычный uColor.
     */
    vec3 surfaceColor =
        uColor.rgb;


    /*
     * Если Material содержит diffuse texture,
     * цвет пикселя текстуры умножается
     * на базовый цвет материала.
     */
    if (uHasDiffuseTexture == 1) {
        surfaceColor *=
            texture(
                uDiffuseTexture,
                vTexCoord
            ).rgb;
    }


    /*
     * Объекты редактора:
     * grid, axes, gizmo и другие элементы,
     * которым освещение не требуется.
     */
    if (uLightingEnabled == 0) {
        FragColor =
            vec4(
                surfaceColor,
                uColor.a
            );

        return;
    }


    /*
     * Нормаль поверхности.
     */
    vec3 normal =
        normalize(
            vNormal
        );


    /*
     * Направление от текущего fragment
     * к точечному источнику света.
     */
    vec3 lightDirection =
        normalize(
            uLightPosition -
            vFragPos
        );


    /*
     * Ambient.
     *
     * Постоянная небольшая составляющая света,
     * благодаря которой неосвещённая сторона
     * объекта не становится полностью чёрной.
     */
    vec3 ambient =
        uAmbientStrength *
        uLightColor *
        uLightIntensity;


    /*
     * Diffuse.
     *
     * Чем сильнее нормаль направлена
     * в сторону источника света,
     * тем ярче поверхность.
     */
    float diffuseFactor =
        max(
            dot(
                normal,
                lightDirection
            ),
            0.0
        );


    vec3 diffuse =
        uDiffuseStrength *
        diffuseFactor *
        uLightColor *
        uLightIntensity;


    /*
     * Направление от fragment к Camera.
     */
    vec3 viewDirection =
        normalize(
            uViewPosition -
            vFragPos
        );


    /*
     * Отражённое направление света.
     */
    vec3 reflectDirection =
        reflect(
            -lightDirection,
            normal
        );


    /*
     * Specular.
     *
     * uShininess определяет,
     * насколько маленьким и концентрированным
     * будет световой блик.
     */
    float specularFactor =
        pow(
            max(
                dot(
                    viewDirection,
                    reflectDirection
                ),
                0.0
            ),
            uShininess
        );


    vec3 specular =
        uSpecularStrength *
        specularFactor *
        uLightColor *
        uLightIntensity;


    /*
     * Полное освещение поверхности.
     */
    vec3 lighting =
        ambient +
        diffuse +
        specular;


    /*
     * Итоговый цвет:
     *
     * освещение
     * *
     * цвет материала
     * *
     * diffuse texture.
     */
    vec3 result =
        lighting *
        surfaceColor;


    FragColor =
        vec4(
            result,
            uColor.a
        );
}