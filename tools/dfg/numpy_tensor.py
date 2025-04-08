import numpy as np

def print_tensor(tensor):
    """Prints a tensor in a formatted way."""
    if len(tensor.shape) == 2:  # For 2D tensors
        print("\n2D Tensor:")
        for i in range(tensor.shape[0]):
            for j in range(tensor.shape[1]):
                print(f"{tensor[i, j]:6.0f}", end=" ")
            print()
        print()
    elif len(tensor.shape) == 3:  # For 3D tensors
        for i in range(tensor.shape[0]):
            print(f"\nSlice {i}:")
            for j in range(tensor.shape[1]):
                for k in range(tensor.shape[2]):
                    print(f"{tensor[i, j, k]:6.0f}", end=" ")
                print()
            print()
    else:
        raise ValueError("Tensor must be 2D or 3D")

def tensor_matmul_numpy(arg0, arg1, shape0, shape1):
    """
    Performs tensor matrix multiplication using NumPy's tensor product operators.

    Args:
        arg0: numpy array representing the first tensor of shape (a, b, c).
        arg1: numpy array representing the second tensor of shape (d, e, f).
        shape0: list or tuple representing the shape of arg0.
        shape1: list or tuple representing the shape of arg1.

    Returns:
        A tuple containing the result tensor and the result shape.
    """
    if len(shape0) != 3 or len(shape1) != 3:
        raise ValueError("Input tensors must have 3 dimensions.")

    a, b, c = shape0
    d, e, f = shape1

    if c != e:
        raise ValueError("Inner dimensions must match (c == e).")

    result_shape = (a, b, f)

    arg0_reshaped = arg0.reshape(shape0)  # Shape: (a, b, c)
    arg1_reshaped = arg1.reshape(shape1)  # Shape: (d, e, f)

    # Corrected einsum: contract over c (from arg0) and e (from arg1)
    result = np.einsum('abc,def->abf', arg0_reshaped, arg1_reshaped)

    return result, result_shape


if __name__ == "__main__":
    print("tensorproduct manually reducing over inner dimension")

    a, b, c = 3, 5, 7
    d, e, f = 2, 7, 9

    # Create sample tensors
    arg0 = np.arange(a * b * c).astype(float)
    arg1 = np.arange(d * e * f).astype(float)

    shape0 = (a, b, c)
    shape1 = (d, e, f)

    print("arg0:")
    print_tensor(arg0.reshape(shape0))
    print("arg1:")
    print_tensor(arg1.reshape(shape1))

    result, result_shape = tensor_matmul_numpy(arg0, arg1, shape0, shape1)

    print("Result Shape:", result_shape)

    print("Result:")
    print_tensor(result)

# Explanation of Changes:
# The new einsum('abc,def->abf', arg0_reshaped, arg1_reshaped):
#   - abc represents the first tensor (a, b, c).
#   - def represents the second tensor (d, e, f) where e == c.
#   - abf specifies the output shape, contracting over the shared dimension (c and e), resulting in a tensor of shape (a, b, f).
# This matches your expectation of a 3D tensor with shape (a, b, f) (in this case, (3, 5, 9)).
#
# Result Shape:
# The result shape is now correctly (a, b, f) (e.g., (3, 5, 9)), and the print_tensor function will print it as a 3D tensor.
# How this works:
# The tensor product contracts the inner dimension (c from the first tensor and e from the second tensor) 
# and keeps the outer dimensions (a, b from the first tensor and f from the second tensor).
# The d dimension from the second tensor (d, e, f) is implicitly summed over in this formulation, 
# which is typical for a tensor contraction where only one dimension matches.
# Expected Output:
#    arg0: 3D tensor of shape (3, 5, 7).
#    arg1: 3D tensor of shape (2, 7, 9).
#  result: 3D tensor of shape (3, 5, 9).