import asyncio
import numpy as np
import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import math

# Initial frame rate for animation
FPS = 10
MIN_FPS = 5
MAX_FPS = 30

# Domain flow graph
DFG = {
    "nodes": [
        {
            "id": "node1",
            "operator": "matmul",
            "constraints": {"i": [0, 4], "j": [0, 4], "k": [0, 4]}
        },
        {
            "id": "node2",
            "operator": "vecadd",
            "constraints": {"i": [0, 4]}
        }
    ],
    "arcs": [
        {
            "source": "node1",
            "target": "node2",
            "temporal_extent": 1,
            "spatial_extent": [1, 0, 0]
        }
    ]
}

def parse_dfg(dfg_data):
    print("Parsing DFG...")
    return dfg_data["nodes"], dfg_data["arcs"]

def enumerate_nodes(nodes):
    print(f"Enumerating {len(nodes)} nodes...")
    return {node["id"]: node for node in nodes}

def expand_domains(nodes):
    print("Expanding domains...")
    index_spaces = {}
    for node_id, node in nodes.items():
        constraints = node["constraints"]
        if node["operator"] == "matmul":
            indices = [(i, j, k) for i in range(constraints["i"][0], constraints["i"][1])
                      for j in range(constraints["j"][0], constraints["j"][1])
                      for k in range(constraints["k"][0], constraints["k"][1])]
            print(f"Node {node_id}: {len(indices)} matmul indices")
        elif node["operator"] == "vecadd":
            indices = [(i,) for i in range(constraints["i"][0], constraints["i"][1])]
            print(f"Node {node_id}: {len(indices)} vecadd indices")
        index_spaces[node_id] = indices
    return index_spaces

def map_to_spacetime(index_spaces, arcs, nodes):
    print("Mapping to spacetime...")
    spacetime_events = {}
    max_time = 0
    tile_size = 2  # 2x2x2 tiles for parallelism
    for node_id, indices in index_spaces.items():
        events = []
        if nodes[node_id]["operator"] == "matmul":
            for i, j, k in indices:
                # Spatial: Spread tiles across lattice
                x = (i / 4.0) + (i // tile_size) * 0.1
                y = (j / 4.0) + (j // tile_size) * 0.1
                z = (k / 4.0) + (k // tile_size) * 0.1
                # Time: Tiled wavefront
                tile_i, tile_j, tile_k = i // tile_size, j // tile_size, k // tile_size
                t = (tile_i + tile_j + tile_k + (i % tile_size + j % tile_size + k % tile_size) / tile_size) / 6.0
                events.append((x, y, z, t))
                # Data dependencies
                a_pos = (x, -0.1, z, t - 0.05)  # A[i,k] below
                b_pos = (-0.1, y, z, t - 0.05)  # B[k,j] left
                c_pos = (x, y, -0.1, t + 0.05)  # C[i,j] after
                events.extend([a_pos, b_pos, c_pos])
                max_time = max(max_time, t + 0.05)
        else:
            for idx in indices:
                i = idx[0]  # Unpack the integer i from (i,)
                x = i / 4.0
                y, z = 0, 0
                t = i / 4.0
                events.append((x, y, z, t))
                max_time = max(max_time, t)
        spacetime_events[node_id] = events
        print(f"Node {node_id}: {len(events)} events, max t={max_time}")

    for arc in arcs:
        target_id = arc["target"]
        source_id = arc["source"]
        temporal_offset = arc["temporal_extent"] / 4.0
        spatial_offset = [x / 4.0 for x in arc["spatial_extent"]]
        source_max_t = max([e[3] for e in spacetime_events[source_id]])
        for i, (x, y, z, t) in enumerate(spacetime_events[target_id]):
            spacetime_events[target_id][i] = (
                x + spatial_offset[0],
                y + spatial_offset[1],
                z + spatial_offset[2],
                t + temporal_offset + source_max_t
            )
        max_time = max(max_time, max([e[3] for e in spacetime_events[target_id]]))
    print(f"Global max time: {max_time}")
    return spacetime_events, max_time

def check_gl_error():
    error = glGetError()
    if error != GL_NO_ERROR:
        print(f"OpenGL error: {error}")
        return False
    return True

def init_pygame():
    print("Initializing Pygame...")
    pygame.init()
    display = (800, 600)
    pygame.display.set_caption("Interactive Domain Flow Matmul Visualization")
    pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
    print("Pygame window created")
    glMatrixMode(GL_PROJECTION)
    gluPerspective(45, display[0] / display[1], 0.1, 50.0)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()
    glTranslatef(0.0, 0.0, -3.0)
    glEnable(GL_DEPTH_TEST)
    glClearColor(0.0, 0.0, 0.0, 1.0)
    glPointSize(10.0)
    check_gl_error()

def update_camera(radius, azimuth, elevation, target=(0.5, 0.5, 0.5)):
    eye_x = target[0] + radius * math.cos(elevation) * math.cos(azimuth)
    eye_y = target[1] + radius * math.sin(elevation)
    eye_z = target[2] + radius * math.cos(elevation) * math.sin(azimuth)
    gluLookAt(eye_x, eye_y, eye_z, target[0], target[1], target[2], 0, 1, 0)

def render_cube(x, y, z, size=0.02):
    vertices = [
        (x-size, y-size, z-size), (x+size, y-size, z-size),
        (x+size, y+size, z-size), (x-size, y+size, z-size),
        (x-size, y-size, z+size), (x+size, y-size, z+size),
        (x+size, y+size, z+size), (x-size, y+size, z+size)
    ]
    edges = [
        (0,1), (1,2), (2,3), (3,0),
        (4,5), (5,6), (6,7), (7,4),
        (0,4), (1,5), (2,6), (3,7)
    ]
    glBegin(GL_LINES)
    for edge in edges:
        for vertex in edge:
            glVertex3f(*vertices[vertex])
    glEnd()

def render_frame(spacetime_events, current_time, nodes, camera_state):
    try:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        update_camera(
            camera_state["radius"],
            camera_state["azimuth"],
            camera_state["elevation"]
        )

        # Grid
        glLineWidth(1.0)
        glColor3f(0.2, 0.2, 0.2)
        glBegin(GL_LINES)
        for i in range(5):
            x = i / 4.0
            glVertex3f(x, 0, 0); glVertex3f(x, 1, 0)
            glVertex3f(x, 0, 0); glVertex3f(x, 0, 1)
            glVertex3f(0, x, 0); glVertex3f(1, x, 0)
            glVertex3f(0, x, 0); glVertex3f(0, x, 1)
            glVertex3f(0, 0, x); glVertex3f(1, 0, x)
            glVertex3f(0, 0, x); glVertex3f(0, 1, x)
        glEnd()

        # Matmul and dependencies
        active_points = 0
        for node_id, events in spacetime_events.items():
            if nodes[node_id]["operator"] == "matmul":
                for i in range(0, len(events), 4):  # Process groups of 4 (op, A, B, C)
                    x, y, z, t = events[i]
                    if abs(t - current_time) < 0.2:
                        # Operation cube
                        glColor3f(1.0, 0.0, 0.0)
                        render_cube(x, y, z)
                        active_points += 1
                        # Dependencies
                        a_x, a_y, a_z, a_t = events[i+1]
                        b_x, b_y, b_z, b_t = events[i+2]
                        c_x, c_y, c_z, c_t = events[i+3]
                        if abs(a_t - current_time) < 0.2:
                            glColor3f(0.0, 0.0, 1.0)
                            render_cube(a_x, a_y, a_z, 0.015)
                        if abs(b_t - current_time) < 0.2:
                            glColor3f(1.0, 1.0, 0.0)
                            render_cube(b_x, b_y, b_z, 0.015)
                        if abs(c_t - current_time) < 0.2:
                            glColor3f(1.0, 1.0, 1.0)
                            render_cube(c_x, c_y, c_z, 0.015)
                        # Dependency lines
                        glLineWidth(2.0)
                        glBegin(GL_LINES)
                        glColor3f(0.5, 0.5, 0.5)
                        glVertex3f(a_x, a_y, a_z); glVertex3f(x, y, z)
                        glVertex3f(b_x, b_y, b_z); glVertex3f(x, y, z)
                        glVertex3f(x, y, z); glVertex3f(c_x, c_y, c_z)
                        glEnd()
            else:
                for x, y, z, t in events:
                    if abs(t - current_time) < 0.2:
                        glColor3f(0.0, 1.0, 0.0)
                        render_cube(x, y, z)
                        active_points += 1
        print(f"Time {current_time:.2f}: {active_points} active points")

        # Arc as data stream
        glLineWidth(3.0)
        glBegin(GL_LINES)
        active_arcs = 0
        for arc in DFG["arcs"]:
            source_events = spacetime_events[arc["source"]]
            target_events = spacetime_events[arc["target"]]
            sx, sy, sz, st = source_events[-1]
            tx, ty, tz, tt = target_events[0]
            if st <= current_time <= tt:
                glColor3f(0.0, 0.0, 1.0)
                glVertex3f(sx, sy, sz)
                glVertex3f(tx, ty, tz)
                active_arcs += 1
        glEnd()
        print(f"Time {current_time:.2f}: {active_arcs} active arcs")

        pygame.display.flip()
        check_gl_error()
    except Exception as e:
        print(f"Rendering error: {e}")
        raise

async def main():
    try:
        init_pygame()
        nodes, arcs = parse_dfg(DFG)
        node_dict = enumerate_nodes(nodes)
        index_spaces = expand_domains(node_dict)
        spacetime_events, max_time = map_to_spacetime(index_spaces, arcs, node_dict)

        camera_state = {
            "radius": 1.5,
            "azimuth": math.radians(45),
            "elevation": math.radians(30),
            "min_radius": 0.5,
            "max_radius": 5.0,
            "dragging": False
        }
        paused = False
        current_fps = FPS

        current_time = 0.0
        clock = pygame.time.Clock()
        frame_count = 0

        print("Starting animation (endless loop)...")
        print("Controls: Left-click and drag to rotate, mouse wheel to zoom, Space to pause/resume, +/= to speed up, - to slow down, Escape or close window to quit")
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    print("Quit event received")
                    pygame.quit()
                    return
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        print("Escape key pressed")
                        pygame.quit()
                        return
                    if event.key == pygame.K_SPACE:
                        paused = not paused
                        print(f"Animation {'paused' if paused else 'resumed'} at time {current_time:.2f}")
                    if event.key in (pygame.K_PLUS, pygame.K_EQUALS):
                        if current_fps < MAX_FPS:
                            current_fps += 5
                            print(f"FPS increased to {current_fps}, cycle time ~{max_time/current_fps:.2f}s")
                        else:
                            print(f"FPS at maximum ({MAX_FPS})")
                    if event.key in (pygame.K_MINUS, pygame.K_KP_MINUS):
                        print(f"Minus key detected, current_fps={current_fps}")
                        if current_fps > MIN_FPS:
                            current_fps -= 10  # Larger decrement for noticeable slowdown
                            print(f"FPS decreased to {current_fps}, cycle time ~{max_time/current_fps:.2f}s")
                        else:
                            print(f"FPS at minimum ({MIN_FPS})")
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1:
                        camera_state["dragging"] = True
                        pygame.mouse.get_rel()
                if event.type == pygame.MOUSEBUTTONUP:
                    if event.button == 1:
                        camera_state["dragging"] = False
                if event.type == pygame.MOUSEMOTION and camera_state["dragging"]:
                    rel_x, rel_y = pygame.mouse.get_rel()
                    camera_state["azimuth"] -= rel_x * 0.01
                    camera_state["elevation"] = min(
                        math.radians(89),
                        max(math.radians(-89), camera_state["elevation"] - rel_y * 0.01)
                    )
                if event.type == pygame.MOUSEWHEEL:
                    camera_state["radius"] = min(
                        camera_state["max_radius"],
                        max(camera_state["min_radius"], camera_state["radius"] - event.y * 0.1)
                    )

            render_frame(spacetime_events, current_time, node_dict, camera_state)
            if not paused:
                time_increment = 1.0 / current_fps
                current_time += time_increment
                if current_time > max_time:
                    print(f"Resetting animation at time {current_time:.2f} to 0.00")
                    current_time = 0.0
            frame_count += 1
            if is_pyodide():
                await asyncio.sleep(1.0 / current_fps)
            else:
                clock.tick(current_fps)
            if frame_count % current_fps == 0:
                print(f"Frame {frame_count}, time {current_time:.2f}, fps={current_fps}, radius={camera_state['radius']:.2f}, azimuth={math.degrees(camera_state['azimuth']):.1f}°, elevation={math.degrees(camera_state['elevation']):.1f}°")
    except Exception as e:
        print(f"Main loop error: {e}")
        raise

def is_pyodide():
    try:
        import js
        print("Detected Pyodide environment")
        return True
    except ImportError:
        return False

if __name__ == "__main__":
    print("Running in local Python environment")
    if is_pyodide():
        asyncio.ensure_future(main())
    else:
        asyncio.run(main())