import numpy as np

def normalize(v):
    """Normalize a vector."""
    norm = np.linalg.norm(v)
    return v / norm if norm > 0 else v

def compute_rotation_matrix(normal, target_dir):
    """Compute rotation matrix to align normal with target_dir."""
    normal = normalize(np.array(normal, dtype=float))
    target_dir = normalize(np.array(target_dir, dtype=float))
    
    # Check if vectors are already aligned or opposite
    dot_product = np.dot(normal, target_dir)
    if np.isclose(dot_product, 1.0):
        return np.eye(3)
    elif np.isclose(dot_product, -1.0):
        return -np.eye(3)
    
    # Compute rotation axis (cross product) and angle
    v = np.cross(normal, target_dir)
    s = np.linalg.norm(v)
    c = dot_product
    v = normalize(v)
    
    # Skew-symmetric matrix for cross product
    vx = np.array([[0, -v[2], v[1]],
                   [v[2], 0, -v[0]],
                   [-v[1], v[0], 0]])
    
    # Rodrigues' rotation formula: R = I + sin(θ)Vx + (1-cos(θ))Vx^2
    theta = np.arccos(c)
    R = np.eye(3) + np.sin(theta) * vx + (1 - c) * vx @ vx
    return R

def transform_constraints(A, b, rotation, translation):
    """Transform constraints Ax <= b under rotation R and translation t."""
    A = np.array(A, dtype=float)
    b = np.array(b, dtype=float)
    translation = np.array(translation, dtype=float)
    
    # Number of constraints
    m = A.shape[0]
    
    # Transform each constraint
    A_new = np.zeros_like(A)
    b_new = np.zeros_like(b)
    
    for i in range(m):
        # Normal vector of the constraint (row of A)
        normal = A[i]
        # Rotate the normal
        normal_new = rotation @ normal
        # Compute new b: b_new = b - A*t
        b_new[i] = b[i] - normal @ translation
        A_new[i] = normal_new
    
    return A_new, b_new

def main():
    # Example inputs
    # Constraints: Ax <= b in native frame (e.g., a unit cube)
    A = np.array([
        [1, 0, 0],   # x <= 1
        [-1, 0, 0],  # -x <= -1
        [0, 1, 0],   # y <= 1
        [0, -1, 0],  # -y <= -1
        [0, 0, 1],   # z <= 1
        [0, 0, -1]   # -z <= -1
    ])
    b = np.array([1, -1, 1, -1, 1, -1])
    
    # Face to align (e.g., face with normal [1, 0, 0])
    face_normal = [1, 0, 0]
    
    # Target directional vector (from another subsystem)
    target_direction = [0, 1, 0]
    
    # Translation vector
    translation = [2, 3, 4]
    
    # Compute rotation matrix
    R = compute_rotation_matrix(face_normal, target_direction)
    
    # Transform constraints
    A_transformed, b_transformed = transform_constraints(A, b, R, translation)
    
    # Output results
    print("Original constraints:")
    print("A =\n", A)
    print("b =\n", b)
    print("\nRotation matrix:")
    print(R)
    print("\nTransformed constraints:")
    print("A_transformed =\n", A_transformed)
    print("b_transformed =\n", b_transformed)

if __name__ == "__main__":
    main()