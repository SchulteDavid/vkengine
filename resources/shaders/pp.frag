#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput inputAlbedo;

layout (binding = 3) uniform LightData {

    vec4 position[32];
    vec4 color[32];

} inLights;

layout (binding = 4) uniform CameraData {

    vec3 position;

} inCamera;

layout (location = 0) out vec4 ppResult;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {

    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

}

float DistributionGGX(vec3 N, vec3 H, float roughness) {

    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {

    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}


vec3 calculateRadiance(vec3 lPos, vec3 lColor, int type, vec3 WorldPos, vec3 V, float falloff) {

    if (type == 0) return vec3(0);
    else if (type == 1) {
        float distance    = length(lPos - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        return lColor * attenuation;
    } else if (type == 2) {

        return lColor;

    } else if (type == 3) {
        float distance    = length(lPos - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        return lColor * attenuation * (0.5*sin(falloff)+1+0.3*cos(falloff*3));

    } else if (type == 4) {

        vec3 lightDir = normalize(lPos - WorldPos);
        float distance    = length(lPos - WorldPos);
        float attenuation = (cos(PI * clamp(acos(dot(lightDir, normalize(-lColor))) / falloff, 0, 1))+1)*0.5 / (distance);
        return vec3(2,2,2) * attenuation;

    }

}

vec3 computeLO(vec3 WorldPos, int i, vec3 V, vec3 N, float roughness, vec3 F0, vec3 albedo, float metallic) {

    // calculate per-light radiance
        vec3 L = normalize(inLights.position[i].xyz - WorldPos);
        //if (types[i] == 2) L = -normalize(inLights.position[i]);
        vec3 H = normalize(V + L);
        vec3 radiance = calculateRadiance(inLights.position[i].xyz, inLights.color[i].rgb, 1, WorldPos, V, 0.2);

        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 nominator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = nominator / max(denominator, 0.001);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);

        return max((kD * albedo / PI + specular) * radiance * NdotL, vec3(0,0,0));

}

vec4 getGColor(float shadow) {

    vec3 Normal = subpassLoad(inputNormal).rgb;

    vec3 camPos     = inCamera.position;
    vec3 WorldPos   = subpassLoad(inputPosition).rgb;
    vec3 albedo     = subpassLoad(inputAlbedo).rgb;
    float metallic  = subpassLoad(inputAlbedo).a;
    float roughness = subpassLoad(inputNormal).a;

    float ao = 1.0;

    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);//
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    Lo = computeLO(WorldPos, 0, V, N, roughness, F0, albedo, metallic) * shadow;
    for(int i = 1; i < 32; ++i)
    {
        Lo += computeLO(WorldPos, i, V, N, roughness, F0, albedo, metallic);
    }
    if (metallic > -1) {

        vec3 L = normalize(reflect(-V, N));

        vec3 H = normalize(V + L);
        vec3 radiance = vec3(0.1);// * texture(skybox, L).rgb;

        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 nominator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular     = nominator / max(denominator, 0.001);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);

        Lo += max((kD * albedo / PI + specular) * radiance * NdotL, vec3(0,0,0));
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    return vec4(color, subpassLoad(inputPosition).a);

}

void main() {

    vec3 light = vec3(0.1);

    vec3 n = normalize(subpassLoad(inputNormal).xyz);
    vec3 p = subpassLoad(inputPosition).xyz;
    vec3 c = inCamera.position - p;

    for (int i = 0; i < 32; ++i) {

        vec3 r = reflect(-c , n);
        vec3 l = inLights.position[i].xyz - p;

        if (inLights.color[i].w >= 1.0) {

            light += clamp(dot(normalize(inLights.position[i].xyz), n), 0, 1) * inLights.color[i].xyz;

        } else {

            //light += clamp(dot(normalize(l), n), 0, 1) * inLights.color[i].xyz / pow(length(l), inLights.position[i].w);

        }

    }

    //light = vec3(0.1);

    //light += 0;//clamp(dot(normalize(inLights.position[0].xyz), n), 0, 1) * inLights.color[0].xyz;

    //ppResult = vec4(inCamera.position, 1);

    ppResult = getGColor(1.0);
    //ppResult = vec4(light, 1) * subpassLoad(inputAlbedo);
    //ppResult = subpassLoad(inputAlbedo);
    //ppResult = subpassLoad(inputNormal);
    //ppResult = vec4(inCamera.position, 1);
    //ppResult = getGColor(1);

}
