#version 330 core

uniform vec4 uColor; // текущий цвет
uniform float uDashFill; // ширина штриха
uniform int uPointMode; // 0 - ничего, 1 - круг, 2 - квадрат
uniform float uPointSoft; // мягкость края точки

// Параметры источника света.
uniform vec3 uLightColor;
uniform vec3 uLightPosition;

// 0 - освещение выключено.
// 1 - освещение включено.
//
// Это позволяет использовать этот же shader для grid,
// осей и gizmo, не применяя к ним освещение.
uniform int uLightingEnabled;

// Данные, полученные из vertex shader.
in vec3 vFragPos;
in vec3 vNormal;

out vec4 FragColor;

void main() {
    if (uPointMode == 1) {
        vec2 p = gl_PointCoord - vec2(0.5);
        float r = length(p);

        float edge0 = 0.5 - uPointSoft;
        float edge1 = 0.5;

        float alpha =
            1.0 - smoothstep(
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
    } else if (uPointMode == 2) { // квадрат
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
            0.5 - uPointSoft;

        float edge1 =
            0.5;

        float alpha =
            1.0 - smoothstep(
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
     * Grid, оси и Move Gizmo не должны
     * рассчитывать освещение.
     */
    if (uLightingEnabled == 0) {
        FragColor = uColor;
        return;
    }

    /*
     * Ambient lighting.
     *
     * Небольшое постоянное освещение поверхности.
     * Благодаря ему части объекта, на которые
     * непосредственно не попадает свет,
     * не становятся полностью чёрными.
     */
    float ambientStrength = 0.15;

    vec3 ambient =
        ambientStrength *
        uLightColor;

    /*
     * Нормализованная нормаль поверхности.
     */
    vec3 normal =
        normalize(vNormal);

    /*
     * Направление ОТ текущего фрагмента
     * К источнику света.
     *
     * Обе позиции находятся в мировом пространстве,
     * поэтому их можно непосредственно вычитать.
     */
    vec3 lightDirection =
        normalize(
            uLightPosition -
            vFragPos
        );

    /*
     * Diffuse lighting.
     *
     * dot() показывает, насколько направление
     * нормали совпадает с направлением на свет.
     *
     * 1.0 — поверхность направлена прямо на свет.
     * 0.0 — свет идёт параллельно поверхности.
     *
     * Отрицательные значения отбрасываются,
     * потому что источник находится за поверхностью.
     */
    float diffuseStrength =
        max(
            dot(
                normal,
                lightDirection
            ),
            0.0
        );

    vec3 diffuse =
        diffuseStrength *
        uLightColor;

    /*
     * Пока используем две составляющие:
     *
     * Ambient + Diffuse.
     *
     * Specular добавим следующим этапом.
     */
    vec3 lighting =
        ambient +
        diffuse;

    /*
     * Рассчитанное освещение применяется
     * к исходному цвету объекта.
     */
    vec3 result =
        lighting *
        uColor.rgb;

    FragColor =
        vec4(
            result,
            uColor.a
        );
}