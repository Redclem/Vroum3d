
#version 460
#extension GL_EXT_ray_query : enable

layout(location = 0) in vec2 i_pos;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;

void main()
{
  // Ray origin on the plane Z = -2.0, pointing directly down +Z toward Z = 0
  vec3 ray_origin = vec3(i_pos.x * 2.0, i_pos.y * 2.0, -2.0);
  vec3 ray_dir    = vec3(0.0, 0.0, 1.0);

  float tMin = 0.001;
  float tMax = 100.0;

  rayQueryEXT rayQuery;
  rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF, ray_origin, 0.01, ray_dir, 100.0);

  rayQueryProceedEXT(rayQuery);

  // Process traversal
  /*while (rayQueryProceedEXT(rayQuery))
  {
      // If geometry is flagged as non-opaque or no-opaque, candidate intersections
      // must be explicitly confirmed to become committed hits
      if (rayQueryGetIntersectionTypeEXT(rayQuery, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
      {
          rayQueryConfirmIntersectionEXT(rayQuery);
      }
  }*/

  // Inspect final committed intersection state
  uint hitType = rayQueryGetIntersectionTypeEXT(rayQuery, true);

  if (hitType == gl_RayQueryCommittedIntersectionTriangleEXT)
  {
      // HIT -> Bright Green (shows UV/barycentric gradient to verify internal hit coordinates)
      vec2 bary = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
      o_color = vec4(bary.x, 1.0, bary.y, 1.0);
  }
  else
  {
      // MISS -> Dark Blue
      o_color = vec4(0.05, 0.05, 0.25, 1.0);
  }
}
