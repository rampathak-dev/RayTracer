struct Camera {
    center: vec4f,
    pixel00: vec4f,
    pixelDeltaU: vec4f,
    pixelDeltaV: vec4f,
    defocusDiskU: vec4f,
    defocusDiskV: vec4f}

struct Environment {
    skyColour: vec4f,
    groundColour: vec4f}

struct RenderSettings {
    recursionDepth: u32,
    samplesPerPixel: u32,
    shutterOpenTime: f32,
    shutterCloseTime: f32}

struct ImageDimensions {
    width: u32,
    height: u32,
    pad0: u32,
    pad1: u32}

struct Texture {
    albedo: vec4f,
    evenIndex: u32,
    oddIndex: u32,
    invScale: f32,
    width: u32,
    height: u32,
    pixelOffset: u32,
    perlinOffset: u32,
    noiseScale: f32,
    distortion: f32,
    turbulenceDepth: u32,
    kind: u32}

struct TextureList {
    count: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    textures: array<Texture>}

struct PixelData {
    maxOffset: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    pixels: array<u32>}

struct PerlinNoiseValues {
    maxOffset: u32,
    pointCount: u32,
    pad0: u32,
    pad1: u32,
    noiseValues: array<vec4f>}

struct PerlinPermutations {
    maxOffset: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    permutations: array<u32>}

struct Material {
    albedo: vec3f,
    pad0: u32,
    textureIndex: u32,
    fuzz: f32,
    etaRatio: f32,
    kind: u32}

struct MaterialList {
    count: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    materials: array<Material>}

struct Volume {
    materialIndex: u32,// phase function
    negInvDensity: f32,
    boundaryIndex: u32,
    boundaryCount: u32,
    kind: u32}

struct VolumeList {
    count: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    volumes: array<Volume>}

struct Object {
    center0: vec4f,
    center1: vec4f,
    Q: vec4f,
    u: vec4f,
    v: vec4f,
    n: vec4f,
    w: vec4f,
    radius: f32,
    time0: f32,
    time1: f32,
    D: f32,
    materialIndex: u32,
    isVolumeBoundary: u32,
    kind: u32}

struct ObjectList {
    count: u32,
    pad0: u32,
    pad1: u32,
    pad2: u32,
    objects: array<Object>}

struct AABB {
    min: vec4f,
    max: vec4f}

struct BVHNode {
    aabb: AABB,
    leftIndex: u32,
    rightIndex: u32,
    objectIndex: u32};

struct BVHNodeList {
    count: u32,
    rootIndex: u32,
    pad0: u32,
    pad1: u32,
    nodes: array<BVHNode>}

struct Ray {
    origin: vec3f,
    direction: vec3f,
    time: f32}

struct HitRecord {
    intersection: vec3f,
    normal: vec3f,
    t: f32,
    u: f32,
    v: f32,
    materialIndex: u32,
    frontFace: bool,
    hit: bool}

struct ScatterRecord {
    ray: Ray,
    albedo: vec3f,
    isRayAbsorbed: bool}

struct RefractRecord {
    direction: vec3f,
    cosTheta: f32,
    isRayRefracted: bool}

const kTMin: f32 = 1e-3f;
const kTMax: f32 = 1e38f;
const kEpsilon: f32 = 1e-8f;
const kPi: f32 = 3.14159265358979323846;

const kTextureKindSolidColour = 0u;
const kTextureKindCheckerTexture = 1u;
const kTextureKindImageTexture = 2u;
const kTextureKindNoiseTexture = 3u;

const kMaterialKindLambertian = 0u;
const kMaterialKindMetal = 1u;
const kMaterialKindDielectric = 2u;
const kMaterialKindDiffuseLight = 3u;
const kMaterialKindIsotropic = 4u;

const kVolumeKindConstantMedium = 0u;

const kObjectKindSphere = 0u;
const kObjectKindMovingSphere = 1u;
const kObjectKindParallelogram = 2u;

const kUint32Sentinel = 0u - 1u;
const kIsVolumeBoundary = kUint32Sentinel;
const kNoObject = kUint32Sentinel;

override kGroupSize: u32;

@group(0) @binding(0) var<uniform> uCamera: Camera;
@group(0) @binding(1) var<uniform> uEnvironment: Environment;
@group(0) @binding(2) var<uniform> uRenderSettings: RenderSettings;
@group(0) @binding(3) var<uniform> uImageDimensions: ImageDimensions;
@group(0) @binding(4) var<storage, read> sTextureList: TextureList;
@group(0) @binding(5) var<storage, read> sPixelData: PixelData;
@group(0) @binding(6) var<storage, read> sPerlinNoiseValues: PerlinNoiseValues;
@group(0) @binding(7) var<storage, read> sPerlinPermutations: PerlinPermutations;
@group(0) @binding(8) var<storage, read> sMaterialList: MaterialList;
@group(0) @binding(9) var<storage, read> sVolumeList: VolumeList;
@group(0) @binding(10) var<storage, read> sObjectList: ObjectList;
@group(0) @binding(11) var<storage, read> sNodeList: BVHNodeList;
@group(0) @binding(12) var<storage, read_write> sOutputBuffer: array<vec4f>;

@compute @workgroup_size(kGroupSize, kGroupSize)
fn CSMain(@builtin(global_invocation_id) id: vec3u) {
    if id.x >= uImageDimensions.width || id.y >= uImageDimensions.height {
        return;
    }

    var colour = vec3f(0, 0, 0);

    for (var s: u32; s < uRenderSettings.samplesPerPixel; s += 1u) {
        var rngState: u32 = ComputeRNGSeed(id.x, id.y, s);

        let ray = RandomRay(id.x, id.y, &rngState);
        colour += RayColour(ray, &rngState);
    }
    colour /= f32(uRenderSettings.samplesPerPixel);

    sOutputBuffer[id.x + id.y * uImageDimensions.width] = vec4f(colour, 0);
}

fn RayColour(originalRay: Ray, rngState: ptr<function, u32>) -> vec3f {
    var ray = originalRay;
    var radiance = vec3f(0, 0, 0);
    var attenuation = vec3f(1, 1, 1);

    var depth = 0u;
    while depth < uRenderSettings.recursionDepth {
        var hitRecord = Hit(ray, kTMin, kTMax, rngState);

        for (var i = 0u; i < sVolumeList.count; i += 1) {
            let volumeHitRecord = VolumeHit(i, ray, kTMin, select(kTMax, hitRecord.t, hitRecord.hit), rngState);

            if volumeHitRecord.hit {
                hitRecord = volumeHitRecord;
            }
        }

        if !hitRecord.hit {
            let unitRayDirection = normalize(ray.direction);
            let interpolation = (unitRayDirection.y + 1) * 0.5;
            radiance += attenuation * ((1 - interpolation) * uEnvironment.groundColour.xyz + interpolation * uEnvironment.skyColour.xyz);

            return radiance;
        }

        var emitted = Emit(hitRecord.materialIndex, hitRecord.u, hitRecord.v, hitRecord.intersection);
        radiance += attenuation * emitted;

        let scatterRecord = DispatchScatter(ray, hitRecord, rngState);
        if scatterRecord.isRayAbsorbed {
            return radiance;
        }

        attenuation *= scatterRecord.albedo;
        ray = scatterRecord.ray;

        depth += 1u;
    }

    return vec3f(0, 0, 0);
}

// linear hit test
/*
fn Hit(ray: Ray, tMin: f32, tMax: f32, rngState: ptr<function, u32>) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    var tClosest = tMax;

    for (var i = 0u; i < sObjectList.count; i += 1u) {
        var newRecord: HitRecord;

        if sObjectList.objects[i].isVolumeBoundary == kIsVolumeBoundary {
            continue;
        }

        newRecord = DispatchHit(i, ray, tMin, tClosest);

        if newRecord.hit {
            record = newRecord;
            tClosest = newRecord.t;
        }
    }

    return record;
}
*/

// bvh hit test
fn Hit(ray: Ray, tMin: f32, tMax: f32, rngState: ptr<function, u32>) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    if sNodeList.count == 0 {
        return record;
    }

    var stackPtr = 1i;
    var stack: array<u32, 32u>;
    stack[0] = sNodeList.rootIndex;

    var tClosest = tMax;

    while stackPtr > 0i {
        stackPtr -= 1;

        let nodeIndex = stack[stackPtr];
        let node = sNodeList.nodes[nodeIndex];

        if !AABBHit(node.aabb, ray, tMin, tClosest) {
            continue;
        }

        if node.objectIndex != kNoObject {
            var newRecord: HitRecord;

            newRecord = DispatchHit(node.objectIndex, ray, tMin, tClosest);

            if newRecord.hit {
                record = newRecord;
                tClosest = newRecord.t;
            }

            continue;
        }

        stack[stackPtr] = node.leftIndex;
        stack[stackPtr + 1] = node.rightIndex;

        stackPtr += 2;
    }

    return record;
}

fn VolumeHit(volumeIndex: u32, ray: Ray, tMin: f32, tMax: f32, rngState: ptr<function, u32>) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    if volumeIndex >= sVolumeList.count {
        return record;
    }

    let kind = sVolumeList.volumes[volumeIndex].kind;

    if kind == kVolumeKindConstantMedium {
        return ConstantMediumHit(volumeIndex, ray, tMin, tMax, rngState);
    }

    return record;
}

fn ConstantMediumHit(volumeIndex: u32, ray: Ray, tMin: f32, tMax: f32, rngState: ptr<function, u32>) -> HitRecord {
    let volume = sVolumeList.volumes[volumeIndex];

    var record0: HitRecord;
    record0.hit = false;
    record0.t = kTMax;

    for (var i = 0u; i < volume.boundaryCount; i += 1) {
        let newRecord = DispatchHit(volume.boundaryIndex + i, ray, -kTMax, kTMax);
        record0.hit = newRecord.hit || record0.hit;
        record0.t = select(record0.t, newRecord.t, (record0.t > newRecord.t) && newRecord.hit);
    }

    if !record0.hit {
        return record0;
    }

    var record1: HitRecord;
    record1.hit = false;
    record1.t = -kTMax;

    for (var i = 0u; i < volume.boundaryCount; i += 1) {
        let newRecord = DispatchHit(volume.boundaryIndex + i, ray, record0.t, kTMax);
        record1.hit = newRecord.hit || record1.hit;
        record1.t = select(record1.t, newRecord.t, (record1.t < newRecord.t) && newRecord.hit);
    }

    if !record1.hit {
        return record1;
    }

    record0.t = max(record0.t, tMin);
    record1.t = min(record1.t, tMax);

    var finalRecord: HitRecord;
    finalRecord.hit = false;

    if record0.t >= record1.t {
        return finalRecord;
    }

    record0.t = max(record0.t, 0);

    let rayLength = length(ray.direction);
    let distanceInsideBoundary = (record1.t - record0.t) * rayLength;
    let hitDistance = volume.negInvDensity * log(RandomFloat(rngState));

    if hitDistance > distanceInsideBoundary {
        return finalRecord;
    }

    finalRecord.hit = true;
    finalRecord.t = record0.t + hitDistance / rayLength;
    finalRecord.intersection = ray.origin + ray.direction * finalRecord.t;
    finalRecord.normal = vec3f(1, 0, 0); // arbitrary
    finalRecord.frontFace = true;              // arbitrary
    finalRecord.materialIndex = volume.materialIndex;

    return finalRecord;
}

fn DispatchHit(objectIndex: u32, ray: Ray, tMin: f32, tMax: f32) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    if objectIndex >= sObjectList.count {
        return record;
    }

    let kind = sObjectList.objects[objectIndex].kind;

    if kind == kObjectKindSphere {
        record = SphereHit(objectIndex, ray, tMin, tMax);
    } else if kind == kObjectKindMovingSphere {
        record = MovingSphereHit(objectIndex, ray, tMin, tMax);
    } else if kind == kObjectKindParallelogram {
        record = ParallelogramHit(objectIndex, ray, tMin, tMax);
    }

    return record;
}

fn AABBHit(aabb: AABB, ray: Ray, tMinIn: f32, tMaxIn: f32) -> bool {
    var tMin = tMinIn;
    var tMax = tMaxIn;

    return AABBCoordHit(aabb.min.x, aabb.max.x, ray.origin.x, ray.direction.x, &tMin, &tMax) &&
    AABBCoordHit(aabb.min.y, aabb.max.y, ray.origin.y, ray.direction.y, &tMin, &tMax) &&
    AABBCoordHit(aabb.min.z, aabb.max.z, ray.origin.z, ray.direction.z, &tMin, &tMax);
}

fn AABBCoordHit(minA: f32, maxA: f32, originA: f32, directionA: f32, tMin: ptr<function , f32>, tMax: ptr<function, f32>) -> bool {
    let invDirectionA = 1 / directionA;

    var t0 = (minA - originA) * invDirectionA;
    var t1 = (maxA - originA) * invDirectionA;

    let temp = t0;
    let swap = invDirectionA < 0;

    t0 = select(t0, t1, swap);
    t1 = select(t1, temp, swap);

    *tMin = select(*tMin, t0, t0 > *tMin);
    *tMax = select(*tMax, t1, t1 < *tMax);

    return *tMax >= *tMin;
}

fn SphereHit(objectIndex: u32, ray: Ray, tMin: f32, tMax: f32) -> HitRecord {
    var record: HitRecord;

    let center = sObjectList.objects[objectIndex].center0;
    let radius = sObjectList.objects[objectIndex].radius;

    let oc = center.xyz - ray.origin;
    let a = dot(ray.direction, ray.direction);
    let h = dot(ray.direction, oc);
    let c = dot(oc, oc) - radius * radius;

    let discriminant = h * h - (a * c);
    let hasRealRoots = discriminant >= 0;

    let sqrtD = sqrt(max(discriminant, 0));
    let invA = 1 / a;

    var t0 = (h - sqrtD) * invA;
    let t1 = (h + sqrtD) * invA;

    let t = select(t1, t0, t0 > tMin && t0 < tMax);

    record.hit = hasRealRoots && (t > tMin && t < tMax);

    if !record.hit {
        return record;
    }

    record.intersection = ray.origin + ray.direction * t;
    record.t = t;

    let outwardNormal = (record.intersection - center.xyz) / radius;

    let uv = ComputeSphereUV(outwardNormal);
    record.u = uv.x;
    record.v = uv.y;

    record.materialIndex = sObjectList.objects[objectIndex].materialIndex;

    record.frontFace = dot(ray.direction, outwardNormal) < 0;
    record.normal = select(-outwardNormal, outwardNormal, record.frontFace);

    return record;
}

fn MovingSphereHit(objectIndex: u32, ray: Ray, tMin: f32, tMax: f32) -> HitRecord {
    var record: HitRecord;

    let movingSphere = sObjectList.objects[objectIndex];

    let center = movingSphere.center0 +
    ((ray.time - movingSphere.time0) / (movingSphere.time1 - movingSphere.time0)) * (movingSphere.center1 - movingSphere.center0);

    let oc = center.xyz - ray.origin;
    let a = dot(ray.direction, ray.direction);
    let h = dot(ray.direction, oc);
    let c = dot(oc, oc) - movingSphere.radius * movingSphere.radius;

    let discriminant = h * h - (a * c);
    let hasRealRoots = discriminant >= 0;

    let sqrtD = sqrt(max(discriminant, 0));
    let invA = 1 / a;

    var t0 = (h - sqrtD) * invA;
    let t1 = (h + sqrtD) * invA;

    let t = select(t1, t0, t0 > tMin && t0 < tMax);

    record.hit = hasRealRoots && (t > tMin && t < tMax);

    if !record.hit {
        return record;
    }

    record.intersection = ray.origin + ray.direction * t;
    record.t = t;

    let outwardNormal = (record.intersection - center.xyz) / movingSphere.radius;

    let uv = ComputeSphereUV(outwardNormal);
    record.u = uv.x;
    record.v = uv.y;

    record.materialIndex = movingSphere.materialIndex;

    record.frontFace = dot(ray.direction, outwardNormal) < 0;
    record.normal = select(-outwardNormal, outwardNormal, record.frontFace);

    return record;
}

fn ParallelogramHit(objectIndex: u32, ray: Ray, tMin: f32, tMax: f32) -> HitRecord {
    var record: HitRecord;
    record.hit = false;

    let parallelogram = sObjectList.objects[objectIndex];
    let denominator = dot(parallelogram.n.xyz, ray.direction);

    if abs(denominator) < kEpsilon {
        return record;
    }

    let t = (parallelogram.D - dot(parallelogram.n.xyz, ray.origin)) / denominator;

    if !(t >= tMin && t <= tMax) {
        return record;
    }

    record.intersection = ray.origin + ray.direction * t;
    let planarHitPointVector = record.intersection - parallelogram.Q.xyz;

    let alpha = dot(parallelogram.w.xyz, cross(planarHitPointVector, parallelogram.v.xyz));
    let beta = dot(parallelogram.w.xyz, cross(parallelogram.u.xyz, planarHitPointVector));

    let hitIsInterior = alpha >= 0 && alpha <= 1 && beta >= 0 && beta <= 1;
    if !hitIsInterior {
        return record;
    }

    record.hit = true;
    record.t = t;
    record.u = alpha;
    record.v = beta;
    record.materialIndex = parallelogram.materialIndex;
    record.frontFace = dot(parallelogram.n.xyz, ray.direction) < 0;
    record.normal = select(-parallelogram.n.xyz, parallelogram.n.xyz, record.frontFace);

    return record;
}

fn ComputeSphereUV(p: vec3f) -> vec2f {
    let theta = acos(-p.y);
    let phi = atan2(-p.z, p.x) + kPi;

    return vec2f(phi / (2 * kPi), theta / kPi);
}

fn Emit(materialIndex: u32, u: f32, v: f32, p: vec3f) -> vec3f {
    let kind = sMaterialList.materials[materialIndex].kind;

    if kind == kMaterialKindDiffuseLight {
        return TextureValue(sMaterialList.materials[materialIndex].textureIndex, u, v, p);
    }

    return vec3f(0, 0, 0);
}

fn DispatchScatter(
    ray: Ray,
    hitRecord: HitRecord,
    rngState: ptr<function, u32>
) -> ScatterRecord {
    if hitRecord.materialIndex >= sMaterialList.count {
        return ScatterRecord(
            Ray(vec3f(0.0), vec3f(0.0), 0.0),
            vec3f(0.0),
            true
        );
    }

    let kind = sMaterialList.materials[hitRecord.materialIndex].kind;

    if kind == kMaterialKindLambertian {
        return ScatterLambertian(hitRecord, ray, rngState);
    }

    if kind == kMaterialKindMetal {
        return ScatterMetal(hitRecord, ray, rngState);
    }

    if kind == kMaterialKindDielectric {
        return ScatterDielectric(hitRecord, ray, rngState);
    }

    if kind == kMaterialKindIsotropic {
        return ScatterIsotropic(hitRecord, ray, rngState);
    }

    var record: ScatterRecord;
    record.isRayAbsorbed = true;

    return record;
}

fn ScatterLambertian(hitRecord: HitRecord, ray: Ray, rngState: ptr<function, u32>) -> ScatterRecord {
    var scattered = hitRecord.normal + RandomUnitVector(rngState);
    scattered = select(scattered, hitRecord.normal, NearZero(scattered));

    return ScatterRecord(
        Ray(hitRecord.intersection, scattered, ray.time),
        TextureValue(
            sMaterialList.materials[hitRecord.materialIndex].textureIndex,
            hitRecord.u, hitRecord.v, hitRecord.intersection
        ),
        false
    );
}

fn ScatterMetal(hitRecord: HitRecord, ray: Ray, rngState: ptr<function, u32>) -> ScatterRecord {
    let albedo = sMaterialList.materials[hitRecord.materialIndex].albedo;
    let fuzz = sMaterialList.materials[hitRecord.materialIndex].fuzz;

    var reflected = Reflect(ray.direction, hitRecord.normal);
    reflected = normalize(reflected) + RandomUnitVector(rngState) * fuzz;

    var scatterRecord: ScatterRecord;

    scatterRecord.albedo = albedo;
    scatterRecord.ray = Ray(hitRecord.intersection, reflected, ray.time);
    scatterRecord.isRayAbsorbed = dot(reflected, hitRecord.normal) <= kEpsilon;

    return scatterRecord;
}

fn ScatterDielectric(hitRecord: HitRecord, ray: Ray, rngState: ptr<function, u32>) -> ScatterRecord {
    var etaRatio = sMaterialList.materials[hitRecord.materialIndex].etaRatio;
    etaRatio = select(1 / etaRatio, etaRatio, hitRecord.frontFace);

    let refractRecord = Refract(ray.direction, hitRecord.normal, etaRatio);
    let probability = SchlickApproximation(etaRatio, refractRecord.cosTheta);
    let reflected = Reflect(ray.direction, hitRecord.normal);

    let direction = select(reflected, refractRecord.direction, refractRecord.isRayRefracted && (probability < RandomFloat(rngState)));

    return ScatterRecord(
        Ray(hitRecord.intersection, direction, ray.time),
        vec3f(1, 1, 1),
        false
    );
}

fn ScatterIsotropic(hitRecord: HitRecord, ray: Ray, rngState: ptr<function, u32>) -> ScatterRecord {
    return ScatterRecord(
        Ray(hitRecord.intersection, RandomUnitVector(rngState), ray.time),
        TextureValue(
            sMaterialList.materials[hitRecord.materialIndex].textureIndex,
            hitRecord.u, hitRecord.v, hitRecord.intersection
        ),
        false
    );
}

fn TextureValue(textureIndex: u32, u: f32, v: f32, p: vec3f) -> vec3f {
    let kind = sTextureList.textures[textureIndex].kind;

    if kind == kTextureKindSolidColour {
        return sTextureList.textures[textureIndex].albedo.xyz;
    }

    if kind == kTextureKindCheckerTexture {
        return CheckerTextureValue(textureIndex, p);
    }

    if kind == kTextureKindImageTexture {
        return ImageTextureValue(textureIndex, u, v);
    }

    if kind == kTextureKindNoiseTexture {
        return NoiseTextureValue(textureIndex, p);
    }

    return vec3f(0, 0, 0);
}

fn CheckerTextureValue(textureIndex: u32, p: vec3f) -> vec3f {
    var index = textureIndex;

    loop {
        let invScale = sTextureList.textures[index].invScale;
        let evenIndex = sTextureList.textures[index].evenIndex;
        let oddIndex = sTextureList.textures[index].oddIndex;

        let xi = i32(floor(invScale * p.x));
        let yi = i32(floor(invScale * p.y));
        let zi = i32(floor(invScale * p.z));

        index = select(oddIndex, evenIndex, (xi + yi + zi) % 2 == 0);

        if sTextureList.textures[index].kind != kTextureKindCheckerTexture {
            break;
        }
    }

    return sTextureList.textures[index].albedo.xyz;
}

fn ImageTextureValue(textureIndex: u32, u: f32, v: f32) -> vec3f {
    let width = sTextureList.textures[textureIndex].width;
    let height = sTextureList.textures[textureIndex].height;
    let pixelOffset = sTextureList.textures[textureIndex].pixelOffset;

    let i = u32(clamp(u, 0.0, 1.0) * f32(width - 1));
    let j = u32(clamp(v, 0.0, 1.0) * f32(height - 1));

    let pixelIndex = pixelOffset + j * width + i;

    if pixelIndex >= sPixelData.maxOffset {
        return vec3f(0, 0, 0);
    }
    let colour = unpack4x8unorm(sPixelData.pixels[pixelIndex]);

    return vec3f(colour.x, colour.y, colour.z);
}

fn NoiseTextureValue(textureIndex: u32, p: vec3f) -> vec3f {
    let scale = sTextureList.textures[textureIndex].noiseScale;
    let distortion = sTextureList.textures[textureIndex].distortion;

    return vec3f(0.5, 0.5, 0.5) * (1 + sin(p.z * scale + distortion * Turbulence(textureIndex, p)));
}

fn Turbulence(textureIndex: u32, p: vec3f) -> f32 {
    let perlinOffset = sTextureList.textures[textureIndex].perlinOffset;
    if perlinOffset > sPerlinNoiseValues.maxOffset {
        return 0;
    }

    let permutationOffset = perlinOffset * 3;
    if permutationOffset > sPerlinPermutations.maxOffset {
        return 0;
    }

    let depth = sTextureList.textures[textureIndex].turbulenceDepth;

    var blend = 0.0;
    var weight = 1.0;
    var currentP = p;

    for (var i = 0u; i < depth; i += 1) {
        blend += weight * Noise(currentP, perlinOffset, permutationOffset);
        currentP *= 2;
        weight *= 0.5;
    }

    return abs(blend);
}

fn Noise(p: vec3f, perlinOffset: u32, permutationOffset: u32) -> f32 {
    let pointCount = sPerlinNoiseValues.pointCount;

    let i = i32(floor(p.x));
    let j = i32(floor(p.y));
    let k = i32(floor(p.z));
    var c: array<vec3f, 8>;

    for (var di = 0i; di < 2i; di += 1i) {
        for (var dj = 0i; dj < 2i; dj += 1i) {
            for (var dk = 0i; dk < 2i; dk += 1i) {
                let x = sPerlinPermutations.permutations[permutationOffset + Wrap(i + di, pointCount)];
                let y = sPerlinPermutations.permutations[permutationOffset + pointCount + Wrap(j + dj, pointCount)];
                let z = sPerlinPermutations.permutations[permutationOffset + 2 * pointCount + Wrap(k + dk, pointCount)];

                c[di + 2 * dj + 4 * dk] = sPerlinNoiseValues.noiseValues[perlinOffset + (x ^ y ^ z)].xyz;
            }
        }
    }

    let u = p.x - f32(i);
    let v = p.y - f32(j);
    let w = p.z - f32(k);

    return PerlinInterpolation(c, u, v, w);
}

fn Wrap(value: i32, pointCount: u32) -> u32 {
    var wrapped = value % i32(pointCount);

    wrapped = select(wrapped, wrapped + i32(pointCount), wrapped < 0);
    return u32(wrapped);
}

fn PerlinInterpolation(c: array<vec3f, 8>, u: f32, v: f32, w: f32) -> f32 {
    // hermitian smoothing : f(x) =  3x^2 - 2x^3
    let uu = u * u * (3 - 2 * u);
    let vv = v * v * (3 - 2 * v);
    let ww = w * w * (3 - 2 * w);

    var blend = 0.0;

    for (var i = 0u; i < 2; i += 1) {
        for (var j = 0u; j < 2; j += 1) {
            for (var k = 0u; k < 2; k += 1) {
                let weightV = vec3f(u - f32(i), v - f32(j), w - f32(k));
                blend += (f32(i) * uu + f32((1 - i)) * (1 - uu)) *
                 (f32(j) * vv + f32((1 - j)) * (1 - vv)) *
                 (f32(k) * ww + f32((1 - k)) * (1 - ww)) * dot(weightV, c[i + j * 2 + k * 4]);
            }
        }
    }

    return blend;
}

fn Reflect(u: vec3f, normal: vec3f) -> vec3f {
    return u - 2 * dot(u, normal) * normal;
}

fn Refract(incident: vec3f, normal: vec3f, etaRatio: f32) -> RefractRecord {
    var record: RefractRecord;

    let unitIncident = normalize(incident);

    let cosTheta = dot(-unitIncident, normal);

    let rPerp = etaRatio * (unitIncident + normal * cosTheta);

    let rParaLengthSqr = 1 - dot(rPerp, rPerp);
    record.isRayRefracted = rParaLengthSqr > 0;

    let rPara = -normal * sqrt(max(rParaLengthSqr, 0));

    record.direction = rPara + rPerp;
    record.cosTheta = cosTheta;
    return record;
}

fn SchlickApproximation(etaRatio: f32, cosTheta: f32) -> f32 {
    var r0 = (1 - etaRatio) / (1 + etaRatio); r0 *= r0;
    return r0 + (1 - r0) * pow(1 - cosTheta, 5);
}

fn NearZero(u: vec3f) -> bool {
    return (abs(u.x) <= kEpsilon) && (abs(u.y) <= kEpsilon) && (abs(u.z) <= kEpsilon);
}

fn RandomRay(x: u32, y: u32, rngState: ptr<function, u32>) -> Ray {
    let offset = SampleUnitSquare(rngState);

    let sample = uCamera.pixel00.xyz +
        ((f32(x) + offset.x) * uCamera.pixelDeltaU.xyz) +
        ((f32(y) + offset.y) * uCamera.pixelDeltaV.xyz);

    let origin = SampleDefocusDisk(rngState);

    let time = uRenderSettings.shutterOpenTime + RandomFloat(rngState) * (uRenderSettings.shutterCloseTime - uRenderSettings.shutterOpenTime);

    return Ray(origin, sample - origin, time);
}

fn RandomUnitVector(rngState: ptr<function, u32>) -> vec3f {
    let z = (RandomFloat(rngState) * 2) - 1;
    let radius = sqrt(1.0 - z * z);
    let theta = RandomFloat(rngState) * 2.0 * kPi;

    let x = radius * cos(theta);
    let y = radius * sin(theta);

    return vec3f(x, y, z);
}

fn SampleDefocusDisk(rngState: ptr<function, u32>) -> vec3f {
    let u = SampleUnitDisk(rngState);
    return uCamera.center.xyz + (u.x * uCamera.defocusDiskU.xyz) + (u.y * uCamera.defocusDiskV.xyz);
}

fn SampleUnitSquare(rngState: ptr<function, u32>) -> vec2f {
    return vec2f(
        RandomFloat(rngState) - 0.5,
        RandomFloat(rngState) - 0.5
    );
}

fn SampleUnitDisk(rngState: ptr<function, u32>) -> vec2f {
    let radius = sqrt(RandomFloat(rngState));
    let theta = RandomFloat(rngState) * 2.0 * kPi;

    let x = radius * cos(theta);
    let y = radius * sin(theta);

    return vec2f(x, y);
}

fn RandomFloat(rngState: ptr<function, u32>) -> f32 {
    let state = *rngState;
    *rngState = state * 747796405u + 2891336453u;

    let word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    let result = (word >> 22u) ^ word;

    return f32(result) / 4294967296.0;
}

fn Hash(x: u32) -> u32 {
    let word = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    return (word >> 22u) ^ word;
}

fn ComputeRNGSeed(x: u32, y: u32, s: u32) -> u32 {
    return Hash(x ^ Hash(y ^ Hash(s)));
}
