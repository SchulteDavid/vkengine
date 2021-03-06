#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PI 3.14159265358979323846

#define VIEWPORT_MAX_LIGHT_COUNT 128

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput inputAlbedo;

layout (binding = 3) uniform LightData {

    int activeCount;
    vec4 position[VIEWPORT_MAX_LIGHT_COUNT];
    vec4 color[VIEWPORT_MAX_LIGHT_COUNT];

} inLights;

layout (binding = 4) uniform CameraData {

    mat4 view;
    mat4 projection;

} inCamera;

layout (binding = 5) uniform samplerCube skyBox;

layout (location = 0) in vec3 uv;

layout (location = 0) out vec4 ppResult;

const vec3 skyColor = vec3(0.688927, 0.839366, 1.0);

vec3 toSkyBoxCoord(vec3 direction) {
  return vec3(direction.x, direction.z, direction.y);
}

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
        if (int(inLights.position[i].w) == 2) L = -normalize(inLights.position[i].xyz);
        vec3 H = normalize(V + L);
        vec3 radiance = calculateRadiance(inLights.position[i].xyz, inLights.color[i].rgb, int(inLights.position[i].w), WorldPos, V, 0.2);

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

    vec3 camPos     = inCamera.view[3].xyz;
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
    for(int i = 1; i < inLights.activeCount; ++i)
    {
        Lo += computeLO(WorldPos, i, V, N, roughness, F0, albedo, metallic);
    }
    if (metallic > -1) {

        vec3 L = normalize(reflect(-V, N));

        vec3 H = normalize(V + L);
        vec3 radiance = texture(skyBox, toSkyBoxCoord(L)).rgb;

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
    color = pow(color, vec3(2.2));

    return vec4(color, subpassLoad(inputPosition).a);

}

const float fov = radians(70.0);
const float aspect = 1280.0 / 720.0;

void main() {

  vec3 light = vec3(0.1);

  vec3 n = normalize(subpassLoad(inputNormal).xyz);
  vec3 p = subpassLoad(inputPosition).xyz;
  vec3 c = inCamera.view[3].xyz - p;
  
  ppResult = getGColor(1.0);
  if (subpassLoad(inputPosition).a <= 0.0) {

    float fov_x = atan(tan(fov/2) * aspect) * 2;

    vec3 camDir = normalize(vec3(sin(fov_x * uv.x),
				 -sin(fov * uv.y),
				 -1));
    
    //vec3 direction = normalize(inCamera.facing + 0.1 * vec3(uv.x, uv.y, 1));
    vec3 direction = normalize((inCamera.view * vec4(camDir, 0.0)).xyz);
    
    ppResult = texture(skyBox, toSkyBoxCoord(direction));
    //ppResult = vec4(direction, 1.0);
      
  }

}
