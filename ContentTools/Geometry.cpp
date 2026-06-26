#include "Geometry.h"

namespace mage::tools {
    namespace {
        using namespace math;
        using namespace DirectX;

        /// <summary>
        /// Calculates normals per vertex for the given mesh
        /// </summary>
        /// <param name="m"></param>
        void recalculate_normals(mesh& m) {
            // construct 2 vectors defining edges that are connected with one and same vertex,
            // calculate cross product
            const u32 num_indices = (u32)m.raw_indices.size();
            m.normals.resize(num_indices); // resize array to number of indices

            for ( u32 i = 0; i < num_indices; ++i ) {
                // get indices per triangle and add to i counter so that next triangle will still be read properly
                const u32 i0 = m.raw_indices[i];
                const u32 i1 = m.raw_indices[++i];
                const u32 i2 = m.raw_indices[++i];

                XMVECTOR v0{ XMLoadFloat3(&m.positions[i0]) };
                XMVECTOR v1{ XMLoadFloat3(&m.positions[i1]) };
                XMVECTOR v2{ XMLoadFloat3(&m.positions[i2]) };

                XMVECTOR e0{ v1 - v0 };
                XMVECTOR e1{ v2 - v0 };
                XMVECTOR n{ XMVector3Normalize(XMVector3Cross(e0, e1)) };

                XMStoreFloat3(&m.normals[i], n);
                // because we were adding to i counter, now we gotta mention normals for this triangle this way
                // i.e. counter i at this point is pointing at vertex v2
                m.normals[i - 1] = m.normals[i];
                m.normals[i - 2] = m.normals[i];
            }
        }

        void process_normals(mesh& m, f32 smooting_angle) {
            // NOTE: smoothing angle is the angle between faces.
            // and here we're dealing with angles between normals, so we need to convert it to the angle that is perpendicular to the plane
            // of a triangle; simply subtracting from pi (rotating by 90 degrees) (after converting to radians of course)

            // using cos(smoothing_angle) to determine if edge is hard. 
            const f32 cos_smoothing_angle = XMScalarCos(pi - smooting_angle * pi / 180.0f);
            // NOTE: if we set smoothing_angle to 180 degrees, it means that if anything would deviate just a little from being perfectly flat,
            // would appear as a hard edge
            const bool is_hard_edge = XMScalarNearEqual(smooting_angle, 180.0f, epsilon);
            // same holds for soft edges
            const bool is_soft_edge = XMScalarNearEqual(smooting_angle, 0.0f, epsilon);

            const u32 num_indices = (u32)m.raw_indices.size();
            const u32 num_vertices = (u32)m.positions.size();
            assert(num_indices && num_vertices);

            m.indices.resize(num_indices);

            utl::vector<utl::vector<u32>> index_ref(num_vertices);
            // for each index pointing, remember position it points to (or rather it's index)
            for ( u32 i = 0; i < num_indices; ++i )
                index_ref[m.raw_indices[i]].emplace_back(i);

            for ( u32 i = 0; i < num_vertices; ++i ) {
                auto& refs = index_ref[i];
                u32 num_refs = (u32)refs.size();
                for ( u32 j = 0; j < num_refs; ++j ) {
                    // this vertex is indicated with index at position refs[j]
                    m.indices[refs[j]] = (u32)m.vertices.size(); // in the first loop, it's 0

                    // increasing size if m.vertices array and filling out detail for currently processed vertex
                    vertex& v =  m.vertices.emplace_back();
                    v.position = m.positions[m.raw_indices[refs[j]]];
                    XMVECTOR n1{ XMLoadFloat3(&m.normals[refs[j]]) };
                    
                    if ( !is_hard_edge ) {
                        // we already got first reference to vertex position (i.e. refs[j])
                        for ( u32 k = j + 1; k < num_refs; ++k ) {
                            // consider following references to same vertex position and see if their normals are diffrent
                            
                            f32 cos_theta = 0.f; // this angle represents the cosine of the angle between normals.

                            // we already got first normal n1 loaded, now we pick the next one
                            XMVECTOR n2{ XMLoadFloat3(&m.normals[refs[k]]) };
                            if ( !is_soft_edge ) {
                                // not multiplying by Reciprocal length of n2, because n2 is already normalized, so length is effectively UNIT LENGTH
                                // NOTE: we're accounting for the possible changes of n1 in this calculation during this loop, therefore making it
                                // not normalized anymore
                                // cos(angle) = dot(n1, n2) / (||n1|| * ||n2||) 
                                XMStoreFloat(&cos_theta, XMVector3Dot(n1, n2) * XMVector3ReciprocalLength(n1));
                            }

                            // if cosine value of cos_theta is bigger than cosine value of smoothing angle
                            if ( is_soft_edge || cos_theta >= cos_smoothing_angle ) {
                                // consider it to be a smooth edge and avarage normals
                                n1 += n2;
                                // therefore these values below should be the same
                                m.indices[refs[k]] = m.indices[refs[j]]; 
                                refs.erase(refs.begin() + k);
                                --num_refs;
                                --k; // number of references has gone down by 1, shifting all the elements in array down
                            }
                        }
                    }
                    XMStoreFloat3(&v.normal, XMVector3Normalize(n1));
                }
            }


        }

        /// <summary>
        /// Process vertices in a way that every single one of them doesn't have UVs that are overlapping with other vertices.
        /// </summary>
        /// <param name="m"></param>
        void process_uvs(mesh& m) {
            utl::vector<vertex> old_vertices;
            old_vertices.swap(m.vertices); // m.vertices is now empty.
            utl::vector<u32> old_indices(m.indices.size());
            old_indices.swap(m.indices);

            const u32 num_vertices = (u32)old_vertices.size();
            const u32 num_indices = (u32)old_indices.size();
            assert(num_vertices && num_indices);

            // vertex references -> same trick as in process_normals() function
            utl::vector<utl::vector<u32>> index_ref(num_vertices);
            for ( u32 i = 0; i < num_indices; ++i )
                index_ref[old_indices[i]].emplace_back(i);

            for ( u32 i = 0; i < num_vertices; ++i ) {
                auto& refs = index_ref[i];
                u32 num_refs = (u32)refs.size();
                for ( u32 j = 0; j < num_refs; ++j ) {
                    m.indices[refs[j]] = (u32)m.vertices.size(); // in first lopp iteration 0 -> set index of processed vertex
                    vertex& v = old_vertices[old_indices[refs[j]]];
                    v.uv = m.uv_sets[0][refs[j]];
                    m.vertices.emplace_back(v); // add processed vertex

                    // check if uv-coordinates for other references are equal (or very closely equal) to what we have in our vertex
                    // then if that's true, we merge indices and we go to the next one
                    for ( u32 k = j + 1; k < num_refs; ++k ) {
                        vec2& uv1{ m.uv_sets[0][refs[k]] };
                        if ( XMScalarNearEqual(v.uv.x, uv1.x, epsilon) &&
                             XMScalarNearEqual(v.uv.y, uv1.y, epsilon) ) {
                            m.indices[refs[k]] = m.indices[refs[j]]; // we nerge indices
                            refs.erase(refs.begin() + k);
                            --num_refs;
                            --k;
                        }
                    }
                }
            }
        }

        void pack_vertices_static(mesh& m) {
            const u32 num_vertices = (u32)m.vertices.size();
            assert(num_vertices);
            m.packed_static_vertices.reserve(num_vertices); // resize vector to avoid unneeded amounts of reallocations

            for ( u32 i = 0; i < num_vertices; ++i ) {
                vertex& v = m.vertices[i];
                const u8 signs = (u8)((v.normal.z > 0.f) << 1);
                // NOTE: remember you got to use same ranges [-1.f, 1.f] in shader to unpack these
                const u16 normal_x = (u16)pack_float<16>(v.normal.x, -1.f, 1.f);
                const u16 normal_y = (u16)pack_float<16>(v.normal.y, -1.f, 1.f);
                // TODO: pack tangents in sign and in x/y components
                m.packed_static_vertices
                    .emplace_back(packed_vertex::static_vertex{
                                    v.position, {0,0,0}, signs,
                                    {normal_x, normal_y}, {},
                                    v.uv
                                  });

            }

        }

        void process_vertices(mesh& m, const geometry_import_settings& settings) {
            assert((m.raw_indices.size() % 3) == 0); // we need triangle-based mesh
            if ( settings.calculate_normals || m.normals.empty() ) {
                recalculate_normals(m);
            }

            // basically getting smooth edges if  there're any
            process_normals(m, settings.smoothing_angle);

            if ( !m.uv_sets.empty() ) {
                process_uvs(m);
            }

            pack_vertices_static(m);
        }

        u64 get_mesh_size(const mesh& m) {
            const u64 num_vertices = m.vertices.size();
            const u64 vertex_buffer_size = sizeof(packed_vertex::static_vertex) * num_vertices;
            const u64 index_size = (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32); // more than 64k? Than we need to index buffer to use u32
            const u64 index_buffer_size = index_size * m.indices.size();
            constexpr u64 sizeu32 = sizeof(u32);

            const u64 size =
                sizeu32 + m.name.size() +       // mesh name length and room for mesh name string
                sizeu32 +                       // mesh id
                sizeu32 +                       // vertex size
                sizeu32 +                       // number of vertices
                sizeu32 +                       // index size (16 bit or 32 bit)
                sizeu32 +                       // number of indices
                sizeof(f32) +                   // LOD treshhold
                vertex_buffer_size +            // room for vertices
                index_buffer_size;              // room for indices

            return size;
        }

        /// <summary>
        /// gets the size to allocate for scene in bytes
        /// </summary>
        /// <param name="scene"></param>
        /// <returns></returns>
        u64 get_scene_size(const scene& scene) {
            constexpr u64 sizeu32 = sizeof(u32);
            u64 size =
                sizeu32 +               // name length
                scene.name.size() +     // room for scene name string
                sizeu32;                // number of LODs

            for ( auto& lod : scene.lod_groups ) {
                u64 lod_size =
                    sizeu32 + lod.name.size() +             // LOD name length
                    sizeu32;                                // number of meshes in this LOD

                for ( auto& m : lod.meshes )
                    lod_size += get_mesh_size(m);

                size += lod_size;
            }
            return size;
        }

        void pack_mesh_data(const mesh& m, u8* const buffer, u64& at) {
            // NOTE: don't delete any at +=, unless you want to have some debugging FUN for no reason
            constexpr u64 sizeu32 = sizeof(u32);
            u32 s = 0;
            // mesh name
            s = (u32)m.name.size(); // size of all characters we got to write into buffer
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;
            memcpy(&buffer[at], m.name.c_str(), s); at += s; // write characters into buffer.

            // lod id
            s = m.lod_id;
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            //vertex size
            constexpr u32 vertex_size = sizeof(packed_vertex::static_vertex);
            s = vertex_size;
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            // number of vertices
            const u32 num_vertices = (u32)m.vertices.size();
            s = num_vertices;
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            // index size (16bit or 32 bit)
            const u32 index_size = (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32);
            s = index_size;
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            // number of indices
            const u32 num_indices = (u32)m.indices.size();
            s = num_indices;
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            // LOD treshold
            memcpy(&buffer[at], &m.lod_treshhold, sizeof(f32)); at += sizeof(f32);

            // vertex data
            s = vertex_size * num_vertices;
            memcpy(&buffer[at], m.packed_static_vertices.data(), s); at += s; // binary copy

            // index data
            s = index_size * num_indices;
            void* data = (void*)m.indices.data();
            utl::vector<u16> indices;

            // convert indices to u16 type if that's the case
            if ( index_size == sizeof(u16) ) {
                indices.resize(num_indices);
                for ( u32 i = 0; i < num_indices; ++i ) indices[i] = (u16)m.indices[i];
                data = (void*)indices.data();
            }
            memcpy(&buffer[at], data, s); at += s;
        }
    } // anonymous namespace

    void process_scene(scene& scene, const geometry_import_settings& settings) {
        for ( auto& lod : scene.lod_groups ) {
            for ( auto& m : lod.meshes ) {
                 process_vertices(m, settings);
            }
        }
    }

    

    void pack_data(const scene& scene, scene_data& data) {
        constexpr u64 sizeu32 = sizeof(u32);

        const u64 scene_size = get_scene_size(scene);
        data.buffer_size = (u32)scene_size;
        data.buffer = (u8*)CoTaskMemAlloc(scene_size);
        assert(data.buffer);

        u8* const buffer = data.buffer;
        u64 at = 0; // offset into array(buffer), we need to move it, to not override smth
        u32 s = 0;  // source for all u32 variables we want to write into this buffer.

        // scene name
        s = (u32)scene.name.size(); // size of all characters we got to write into buffer
        memcpy(&buffer[at], &s, sizeu32); at += sizeu32;
        memcpy(&buffer[at], scene.name.c_str(), s); at += s; // write characters into buffer.

        // number of LODs
        s = (u32)scene.lod_groups.size();
        memcpy(&buffer[at], &s, sizeu32); at += sizeu32;
        for ( auto& lod : scene.lod_groups ) {
            // LOD name
            s = (u32)lod.name.size();
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;
            memcpy(&buffer[at], lod.name.c_str(), s); at += s; // kinda same as with scene name

            // number of meshes in this LOD
            s = (u32)lod.meshes.size();
            memcpy(&buffer[at], &s, sizeu32); at += sizeu32;

            for ( auto& m : lod.meshes ) {
                pack_mesh_data(m, buffer, at);
            }
        }

        assert(scene_size == at); // just make sure we wrote the amount of bytes we wanted to.
    }
}