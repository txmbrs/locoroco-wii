# External Wii map format v1

File: assets/maps/st_flower01.lrcmap. All numbers are big endian.
Header: LRCMAP01 (8 bytes), version u32=1, payload byte count u32, FNV-1a u32.
FNV-1a uses offset basis 2166136261 and multiplier 16777619 over the full payload.

Payload: eight u32 counts: vertices, pieces, edges, bodies, fruits, flies, wind, textures.
Then five Vec2 f32 pairs: spawn, goal, left opening flower, right flower, MuiMui.
The following record arrays occur in the same order as their counts:

- Vertex: x,y,u,v f32; r,g,b,a u8 (20 bytes).
- Piece: first,count,texture,body,fruit,repeat i32; z,minX,minY,maxX,maxY f32 (44).
- Edge: ax,ay,bx,by f32; body i32; friction f32 (24).
- Body: kind i32; x,y f32; native source object ID i32 (16).
- Fruit: root x,y f32; body i32; center x,y f32 (20).
- Fly: x,y f32; pickory value i32 (12).
- Wind point: x,y f32 (8).
- Each texture: width,height,byte count u32, zero padding up to next absolute
  file offset divisible by 32, then tiled GX_RGBA8 bytes (width*height*4).

Coordinates are baked scene hierarchy transforms, PSP-to-Wii scale 6, Y inverted.
A body/fruit index of -1 denotes no attachment. Piece geometry is already in
world coordinates; runtime body transforms apply a delta around the stored pivot.

The loader bounds counts, lengths and indices, rejects invalid floats/textures,
and validates the payload checksum before initializing the level. The format
contains data, never executable PSP handlers. The loader has no embedded map fallback.
A changed map file does not require rebuilding the Wii executable.
