import asyncio
import json
import numpy as np
import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import platform as sys_platform
import math

# Frame rate for animation
FPS = 10  # Slower for clarity

# Sample domain flow graph
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
    """Parse the domain flow graph into nodes and arcs."""
    print("Parsing DFG...")
    return dfg_data["nodes"], dfg_data["arcs"]

def enumerate_nodes(nodes):
    """Enumerate nodes and their properties."""
    print(f"Enumerating {len(nodes)} nodes...")
    node_dict = {node["id"]: node for node in nodes}
    return node_dict

def expand_domains(nodes):
    """Expand each node's computational domain into index space."""
    print("Expanding domains...")
    index_spaces = {}
    for node_id, node in nodes.items():
        constraints = node["constraints"]
        if node["operator"] == "matmul":
            i_range = range(constraints["i"][0], constraints["i"][1])
            j_range = range(constraints["j"][0], constraints["j"][1])
            k_range = range(constraints["k"][0], constraints["k"][1])
            indices = [(i, j, k) for i in i_range for j in j_range for k in k_range]
            print(f"Node {node_id}: {len(indices)} matmul indices")
        elif node["operator"] == "vecadd":
            i_range = range(constraints["i"][0], constraints["i"][1])
            indices = [(i,) for i in i_range]
            print(f"Node {node_id}: {len(indices)} vecadd indices")
        index_spaces[node_id] = indices
    return index_spaces

def map_to_spacetime(index_spaces, arcs, nodes):
    """Map index spaces to computational spacetime with wavefront schedule."""
    print("Mapping to spacetime...")
    spacetime_events = {}
    max_time = 0
    for node_id, indices in index_spaces.items():
        events = []
        for idx in indices:
            if nodes[node_id]["operator"] == "matmul":
                i, j, k = idx
                x, y, z = i / 4.0, j / 4.0, k / 4.0  # Spatial grid
                t = (i + j + k) / 12.0  # Wavefront: t = (i + j + k) scaled
            else:
                i = idx[0]
                x, y, z = i / 4.0, 0, 0
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
    """Check for OpenGL errors."""
    error = glGetError()
    if error != GL_NO_ERROR:
        print(f"OpenGL error: {error}")
        return False
    return True

def init_pygame():
    """Initialize Pygame and OpenGL."""
    try:
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
    except Exception as e:
        print(f"Failed to initialize Pygame/OpenGL: {e}")
        raise

def update_camera(radius, azimuth, elevation, target=(0.5, 0.5, 0.5)):
    """Update the camera position based on spherical coordinates."""
    # Convert spherical coordinates to Cartesian
    eye_x = target[0] + radius * math.cos(elevation) * math.cos(azimuth)
    eye_y = target[1] + radius * math.sin(elevation)
    eye_z = target[2] + radius * math.cos(elevation) * math.sin(azimuth)
    gluLookAt(
        eye_x, eye_y, eye_z,
        target[0], target[1], target[2],
        0, 1, 0  # Up vector
    )

def render_frame(spacetime_events, current_time, nodes, camera_state):
    """Render the 3D scene for the current time step."""
    try:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glLoadIdentity()
        update_camera(
            camera_state["radius"],
            camera_state["azimuth"],
            camera_state["elevation"]
        )

        # Draw grid lines for reference
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

        # Draw matmul points with dependency lines
        glPointSize(20.0)
        glBegin(GL_POINTS)
        active_points = 0
        for node_id, events in spacetime_events.items():
            if nodes[node_id]["operator"] == "matmul":
                for x, y, z, t in events:
                    if abs(t - current_time) < 0.2:
                        glColor3f(1.0, 0.0, 0.0)
                        glVertex3f(x, y, z)
                        active_points += 1
            else:
                for x, y, z, t in events:
                    if abs(t - current_time) < 0.2:
                        glColor3f(0.0, 1.0, 0.0)
                        glVertex3f(x, y, z)
                        active_points += 1
        glEnd()

        # Draw dependency lines for matmul (simplified: show A[i,k] to operation)
        glLineWidth(2.0)
        glBegin(GL_LINES)
        for node_id, events in spacetime_events.items():
            if nodes[node_id]["operator"] == "matmul":
                for x, y, z, t in events:
                    if abs(t - current_time) < 0.2:
                        glColor3f(0.5, 0.5, 0.5)
                        glVertex3f(x, 0, z)
                        glVertex3f(x, y, z)
        glEnd()
        check_gl_error()
        print(f"Time {current_time:.2f}: {active_points} active points")

        # Draw arcs
        glLineWidth(3.0)
        glBegin(GL_LINES)
        active_arcs = 0
        for arc in DFG["arcs"]:
            source_events = spacetime_events[arc["source"]]
            target_events = spacetime_events[arc["target"]]
            if source_events and target_events:
                sx, sy, sz, st = source_events[-1]
                tx, ty, tz, tt = target_events[0]
                if st <= current_time <= tt:
                    glColor3f(0.0, 0.0, 1.0)
                    glVertex3f(sx, sy, sz)
                    glVertex3f(tx, ty, tz)
                    active_arcs += 1
        glEnd()
        check_gl_error()
        print(f"Time {current_time:.2f}: {active_arcs} active arcs")

        pygame.display.flip()
    except Exception as e:
        print(f"Rendering error: {e}")
        raise

async def main():
    """Main loop for visualization."""
    try:
        init_pygame()
        nodes, arcs = parse_dfg(DFG)
        node_dict = enumerate_nodes(nodes)
        index_spaces = expand_domains(node_dict)
        spacetime_events, max_time = map_to_spacetime(index_spaces, arcs, node_dict)

        # Camera state
        camera_state = {
            "radius": 1.5,  # Distance from target
            "azimuth": math.radians(45),  # Horizontal angle
            "elevation": math.radians(30),  # Vertical angle
            "min_radius": 0.5,
            "max_radius": 5.0,
            "dragging": False
        }

        current_time = 0.0
        clock = pygame.time.Clock()
        frame_count = 0

        print("Starting animation...")
        print("Controls: Left-click and drag to rotate, mouse wheel to zoom, Escape to quit")
        while current_time <= max_time + 5:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    print("Quit event received")
                    pygame.quit()
                    return
                if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                    print("Escape key pressed")
                    pygame.quit()
                    return
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if event.button == 1:  # Left button
                        camera_state["dragging"] = True
                        pygame.mouse.get_rel()  # Reset relative motion
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
            current_time += 1.0 / FPS
            frame_count += 1
            if is_pyodide():
                await asyncio.sleep(1.0 / FPS)
            else:
                clock.tick(FPS)
            if frame_count % FPS == 0:
                print(f"Frame {frame_count}, time {current_time:.2f}, radius={camera_state['radius']:.2f}, azimuth={math.degrees(camera_state['azimuth']):.1f}°, elevation={math.degrees(camera_state['elevation']):.1f}°")
    except Exception as e:
        print(f"Main loop error: {e}")
        raise

def is_pyodide():
    """Check if running in a Pyodide environment."""
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