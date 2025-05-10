import numpy as np

def rotate_x_minus_90():
    """Returns the rotation matrix for -90 degrees around the x-axis."""
    return np.array([[1, 0, 0],
                     [0, 0, 1],
                     [0, -1, 0]])

def transform_hull(vertices, m, n, k):
    """Transforms the convex hull as described."""
    original_vertices = np.array(vertices)
    centroid = np.array([m/2, n/2, k/2])
    rotation_matrix = rotate_x_minus_90()
    translation_anchor = np.array([m/2, k/2, -n/2])
    transformed_vertices = []
    for v in original_vertices:
        v_centered = v - centroid
        v_rotated = np.dot(rotation_matrix, v_centered)
        v_transformed = v_rotated + translation_anchor
        transformed_vertices.append(v_transformed.tolist())
    return transformed_vertices

# Dimensions of the prisms
m = 8
n = 4
k = 6

# Original vertices of the first prism
vertices1 = [
    [0, 0, 0], [m, 0, 0], [m, 0, k], [0, 0, k],
    [0, n, k], [0, n, 0], [m, n, 0], [m, n, k]
]

# Transform the first prism
transformed_vertices1 = transform_hull(vertices1, m, n, k)
print("Transformed vertices of the first prism:")
for v in transformed_vertices1:
    print(f"({v[0]:.1f}, {v[1]:.1f}, {v[2]:.1f})")

# Original vertices of the second prism (same as the first in local frame)
vertices2 = vertices1

# Transform the second prism (same rotation and anchoring)
transformed_vertices2_reoriented = transform_hull(vertices2, m, n, k)

# Translate the second prism to the right by k + 1 in the global y-direction
translation_second = np.array([0, k + 1, 0])
transformed_vertices2_final = [(np.array(v) + translation_second).tolist() for v in transformed_vertices2_reoriented]

print("\nTransformed vertices of the second prism:")
for v in transformed_vertices2_final:
    print(f"({v[0]:.1f}, {v[1]:.1f}, {v[2]:.1f})")

# Expected output for the first prism (based on your example with a different order)
expected_v0_prime = (0, 0, n)   # Assuming your 'n' corresponds to our 'k' dimension after rotation
expected_v1_prime = (m, 0, n)
expected_v2_prime = (m, k, n)
expected_v3_prime = (0, k, n)
expected_v4_prime = (0, k, 0)
expected_v5_prime = (0, 0, 0)
expected_v6_prime = (m, 0, 0)
expected_v7_prime = (m, k, 0)

print("\nExpected transformed vertices of the first prism (based on your example):")
print(f"v0': {expected_v0_prime}")
print(f"v1': {expected_v1_prime}")
print(f"v2': {expected_v2_prime}")
print(f"v3': {expected_v3_prime}")
print(f"v4': {expected_v4_prime}")
print(f"v5': {expected_v5_prime}")
print(f"v6': {expected_v6_prime}")
print(f"v7': {expected_v7_prime}")

# Expected output for the second prism (shifted by k+1 in y)
expected_v0_prime_2 = (0, k + 1, n)
expected_v1_prime_2 = (m, k + 1, n)
expected_v2_prime_2 = (m, 2*k + 1, n)
expected_v3_prime_2 = (0, 2*k + 1, n)
expected_v4_prime_2 = (0, 2*k + 1, 0)
expected_v5_prime_2 = (0, k + 1, 0)
expected_v6_prime_2 = (m, k + 1, 0)
expected_v7_prime_2 = (m, 2*k + 1, 0)

print("\nExpected transformed vertices of the second prism:")
print(f"v0': {expected_v0_prime_2}")
print(f"v1': {expected_v1_prime_2}")
print(f"v2': {expected_v2_prime_2}")
print(f"v3': {expected_v3_prime_2}")
print(f"v4': {expected_v4_prime_2}")
print(f"v5': {expected_v5_prime_2}")
print(f"v6': {expected_v6_prime_2}")
print(f"v7': {expected_v7_prime_2}")
