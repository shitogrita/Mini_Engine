#version 330 core

#define MAX_POINT_LIGHTS_PER_PASS 8

/**
 * @brief Данные одного Point Light.
 *
 * Position приходит из Transform SceneObject.
 * Color и Intensity хранятся в компоненте PointLight.
 */
struct PointLightData {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform vec4 uColor;
uniform float uDashFill;
uniform int uPointMode;
uniform float uPointSoft;

/**
 * @brief Активные Point Light текущего render pass.
 */
uniform PointLightData uPointLights[MAX_POINT_LIGHTS_PER_PASS];
uniform int uPointLightCount;

uniform vec3 uViewPosition;

uniform float uAmbientStrength;
uniform float uDiffuseStrength;
uniform float uSpecularStrength;
uniform float uShininess;

uniform int uLightingEnabled;

uniform sampler2D uDiffuseTexture;
uniform int uHasDiffuseTexture;

/**
 * @brief Depth Cubemap для Point Shadow Mapping.
 *
 * uShadowLightIndex:
 * -1  — в текущем кадре shadow map отсутствует;
 * >=0 — индекс Point Light, которому принадлежит shadow map.
 */
uniform samplerCube uPointShadowMap;
uniform int uShadowLightIndex;
uniform float uShadowFarPlane;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

/**
 * @brief Вычисляет коэффициент тени Point Light.
 *
 * Depth Cubemap хранит нормализованное расстояние
 * от Point Light до ближайшей поверхности.
 *
 * Текущее расстояние fragment до источника сравнивается
 * с расстоянием, сохранённым в Cubemap.
 *
 * Для сглаживания границы используется PCF.
 *
 * @param lightIndex Индекс Point Light.
 * @param normal Нормаль поверхности.
 * @param lightDirection Направление от fragment к источнику.
 *
 * @return 0.0 — fragment освещён.
 * @return 1.0 — fragment полностью находится в тени.
 */
float CalculatePointShadow(int lightIndex, vec3 normal, vec3 lightDirection) {
    if (uShadowLightIndex < 0 || lightIndex != uShadowLightIndex) {
        return 0.0;
    }

    vec3 fragmentToLight = vFragPos - uPointLights[lightIndex].position;
    float currentDepth = length(fragmentToLight);

    /**
     * Bias уменьшает Shadow Acne.
     *
     * На поверхности под большим углом
     * bias немного увеличивается.
     */
    float bias = max(
        0.025 * (1.0 - dot(normal, lightDirection)),
        0.005
    );

    /**
     * Направления PCF sampling вокруг
     * исходного направления Cubemap.
     */
    vec3 sampleOffsets[20] = vec3[](
        vec3( 1.0,  1.0,  1.0),
        vec3( 1.0, -1.0,  1.0),
        vec3(-1.0, -1.0,  1.0),
        vec3(-1.0,  1.0,  1.0),

        vec3( 1.0,  1.0, -1.0),
        vec3( 1.0, -1.0, -1.0),
        vec3(-1.0, -1.0, -1.0),
        vec3(-1.0,  1.0, -1.0),

        vec3( 1.0,  1.0,  0.0),
        vec3( 1.0, -1.0,  0.0),
        vec3(-1.0, -1.0,  0.0),
        vec3(-1.0,  1.0,  0.0),

        vec3( 1.0,  0.0,  1.0),
        vec3(-1.0,  0.0,  1.0),
        vec3( 1.0,  0.0, -1.0),
        vec3(-1.0,  0.0, -1.0),

        vec3( 0.0,  1.0,  1.0),
        vec3( 0.0, -1.0,  1.0),
        vec3( 0.0, -1.0, -1.0),
        vec3( 0.0,  1.0, -1.0)
    );

    float viewDistance = length(uViewPosition - vFragPos);

    /**
     * Радиус PCF немного увеличивается
     * для удалённых fragment.
     */
    float diskRadius = (1.0 + viewDistance / uShadowFarPlane) / 35.0;

    float shadow = 0.0;

    for (int sampleIndex = 0; sampleIndex < 20; ++sampleIndex) {
        float closestDepth = texture(
            uPointShadowMap,
            fragmentToLight + sampleOffsets[sampleIndex] * diskRadius
        ).r;

        /**
         * В Cubemap глубина записана как:
         *
         * distance / uShadowFarPlane
         *
         * поэтому возвращаем её обратно
         * в World Space distance.
         */
        closestDepth *= uShadowFarPlane;

        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    return shadow / 20.0;
}

/**
 * @brief Вычисляет вклад одного Point Light.
 *
 * Модель освещения:
 *
 * ambient + diffuse + specular.
 *
 * Shadow применяется только к diffuse и specular.
 * Ambient остаётся видимым даже внутри тени.
 *
 * @param lightIndex Индекс Point Light.
 * @param normal Нормаль поверхности.
 * @param viewDirection Направление от fragment к Camera.
 *
 * @return Вклад источника в итоговое освещение.
 */
vec3 CalculatePointLight(int lightIndex, vec3 normal, vec3 viewDirection) {
    PointLightData light = uPointLights[lightIndex];

    vec3 lightDirection = normalize(
        light.position - vFragPos
    );

    /**
     * Ambient.
     */
    vec3 ambient =
        uAmbientStrength *
        light.color *
        light.intensity;

    /**
     * Diffuse.
     */
    float diffuseFactor = max(
        dot(normal, lightDirection),
        0.0
    );

    vec3 diffuse =
        uDiffuseStrength *
        diffuseFactor *
        light.color *
        light.intensity;

    /**
     * Specular.
     */
    vec3 reflectDirection = reflect(
        -lightDirection,
        normal
    );

    float specularFactor = pow(
        max(
            dot(viewDirection, reflectDirection),
            0.0
        ),
        uShininess
    );

    vec3 specular =
        uSpecularStrength *
        specularFactor *
        light.color *
        light.intensity;

    /**
     * Для обычных Point Light функция вернёт 0.
     *
     * Только uShadowLightIndex использует
     * Depth Cubemap.
     */
    float shadow = CalculatePointShadow(
        lightIndex,
        normal,
        lightDirection
    );

    return ambient + (1.0 - shadow) * (diffuse + specular);
}

/**
 * @brief Главная функция Fragment Shader.
 *
 * Обрабатывает:
 * - editor points;
 * - Material color;
 * - diffuse texture;
 * - несколько Point Light;
 * - Point Shadow Mapping.
 */
void main() {
    /**
     * Круглая editor point.
     */
    if (uPointMode == 1) {
        vec2 p = gl_PointCoord - vec2(0.5);
        float r = length(p);

        float alpha = 1.0 - smoothstep(
            0.5 - uPointSoft,
            0.5,
            r
        );

        if (alpha <= 0.0) {
            discard;
        }

        FragColor = vec4(
            uColor.rgb,
            uColor.a * alpha
        );

        return;
    }

    /**
     * Квадратная editor point.
     */
    if (uPointMode == 2) {
        vec2 p = abs(
            gl_PointCoord - vec2(0.5)
        );

        float d = max(
            p.x,
            p.y
        );

        float alpha = 1.0 - smoothstep(
            0.5 - uPointSoft,
            0.5,
            d
        );

        if (alpha <= 0.0) {
            discard;
        }

        FragColor = vec4(
            uColor.rgb,
            uColor.a * alpha
        );

        return;
    }

    /**
     * Базовый цвет материала.
     */
    vec3 surfaceColor = uColor.rgb;

    if (uHasDiffuseTexture == 1) {
        surfaceColor *= texture(
            uDiffuseTexture,
            vTexCoord
        ).rgb;
    }

    /**
     * Grid, Gizmo, axes и другие Editor helpers
     * освещение и Shadow Mapping не используют.
     */
    if (uLightingEnabled == 0) {
        FragColor = vec4(
            surfaceColor,
            uColor.a
        );

        return;
    }

    vec3 normal = normalize(vNormal);

    vec3 viewDirection = normalize(
        uViewPosition - vFragPos
    );

    /**
     * Суммируем вклад всех активных Point Light.
     *
     * Shadow применяется внутри CalculatePointLight()
     * только для uShadowLightIndex.
     */
    vec3 lighting = vec3(0.0);

    for (int lightIndex = 0; lightIndex < uPointLightCount; ++lightIndex) {
        lighting += CalculatePointLight(
            lightIndex,
            normal,
            viewDirection
        );
    }

    FragColor = vec4(
        lighting * surfaceColor,
        uColor.a
    );
}