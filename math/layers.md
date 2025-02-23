# Basic Layer math

Starting with a simple one layer MLP network, we can elucidate the concepts.

An MLP is represented by the following operators:

- input tensor: (shape=(batch_size, input_size= $n$ ))
- Dense(weight tensor(shape=(batch_size, nrOfNeurons= $m$ )):  $$y = W^T * x$$
- bias(shape=(batch_size, nrOfNeurons= $m$ ))
- Activation(shape=(batch_size, nrOfNeurons= $m$ ))

## MLP Layer: $n$ inputs, and $m$ outputs

$$\eqalign{
z_i^{(1)} = \sigma(\sum_{j=1}^n W_{ij}^{(1)}x_j + b_i^{(1)}) \quad \text{for } i = 1,\dots,m 
}$$

Where:

- $z^{(1)}$ is the output of the first layer
- $\sigma$ is the activation function (commonly ReLU, sigmoid, or tanh)
- $W^{(1)}$ is the weight matrix for the first layer
- $x$ is the input vector
- $b^{(1)}$ is the bias vector for the first layer


# MLP layer math

The equation $Y = W^T * X + B$ is a standard way to represent the computation within a single layer of a Deep Neural Network (DNN), where:

*   $X$ represents the input to the layer.  It's a vector.
*   $W^T$ is the transpose of the weight matrix W.  W is a matrix of weights associated with the connections between the inputs and the neurons in this layer. Transposing it (Wᵀ) is necessary for the matrix multiplication to work correctly.
*   $Y$ is the output of the layer, also a vector.
*   $B$ is the bias vector.  It's added to the result of the matrix multiplication.

Here's a breakdown of why this equation is used and what each part does:

1.  **Weighted Sum of Inputs:** The core operation is the matrix multiplication WᵀX.  This calculates a weighted sum of the inputs. Each element in the output vector Y corresponds to a neuron in the layer.  The value of that element is calculated by taking the dot product of the input vector X with a *row* of the weight matrix W (which becomes a column when you transpose W to Wᵀ).  Each weight in that row determines how much influence the corresponding input has on that particular neuron's activation.

2.  **Weights (W):** The weights are the learnable parameters of the neural network.  They are adjusted during the training process to minimize the error between the network's predictions and the actual targets.  Each weight represents the strength of the connection between an input and a neuron.  A higher absolute value of a weight means a stronger connection.

3.  **Bias (B):** The bias term B is also a learnable parameter. It's added to the weighted sum.  The bias allows each neuron to have an activation even when all the inputs are zero.  It shifts the activation function, allowing the neuron to learn more complex patterns.  Without a bias, the neuron's output would always be zero if all inputs were zero, severely limiting the network's capabilities.

4.  **Matrix Multiplication:** Matrix multiplication is an efficient way to perform the weighted sum for all neurons in the layer simultaneously.  It compactly expresses the calculation for all the connections in the layer.

5.  **Activation Function (Not Explicitly in the Equation):**  It's important to note that the equation Y = WᵀX + B only represents the *linear transformation* part of a layer.  In a typical DNN, this linear transformation is followed by a *non-linear activation function* (e.g., ReLU, sigmoid, tanh). This activation function is applied element-wise to the output vector Y.  The activation function introduces non-linearity, which is crucial for the network to learn complex patterns.  Without non-linearity, a DNN would just be a series of linear transformations, and it could be proven that any series of linear transformations can be represented by a single linear transformation, thus losing all the added representational power of multiple layers.

**In summary:** The equation Y = WᵀX + B represents the weighted sum of inputs and the addition of a bias within a single layer of a DNN. This linear transformation is then followed by a non-linear activation function.  The weights and biases are the parameters that the network learns during training, allowing it to map inputs to outputs in a complex and non-linear way.

# Vectors are column vectors

In the standard representation of neural networks and the equation $Y = W^T * X + B$,  $B$ is a column vector, and when $X$ is a column vector, then $Y$ will be a column vectors. 
However, $X$ can also be a set of column vectors, that is a matrix, or 2D tensor, which will make the result $Y$ a 2D tensor as well. The $B$ column vector will then need to be
broadcast among the set of column vectors represented by the 2D tensor $W^T * X$. 

Here's why:

*   **X (Input):**  Think of X as representing a single data point or example.  It's a collection of features, and it's most naturally represented as a column.  Each element in the column corresponds to a feature.

*   **Y (Output):** Y represents the output of the layer for that specific input X.  Each element in the column vector Y corresponds to the activation of a neuron in that layer.

*   **B (Bias):** The bias B is added element-wise to the result of the matrix multiplication WᵀX.  For this addition to work correctly, B needs to have the same dimensions as the result, which will be a column vector if X is a column vector and Wᵀ is constructed accordingly.

*   **Wᵀ (Transposed Weight Matrix):** The dimensions of Wᵀ are such that when you multiply it by the column vector X, you get another column vector.  If X is *m x 1* (m rows, 1 column), then Wᵀ will be *n x m* so that the result of WᵀX is *n x 1*.  *n* is the number of neurons in the layer.  The original weight matrix W (before the transpose) would then be *m x n*.

**Why column vectors?**

This convention is quite standard in machine learning and linear algebra when dealing with data points and features.  It makes the matrix multiplication work out neatly.  You can think of each column of a data matrix (when you have multiple data points) as representing a single data point, and this data point is represented by the column vector X in this equation.

While you *could* technically represent things with row vectors, it would require different matrix dimensions and transposes, and it's just not the commonly accepted standard.  Sticking with the column vector convention makes it much easier to understand and work with neural network code and literature.



# Dynamics Tensor Shapes and dimensional broadcasts

if x is a batch size number of vectors, then w * x is a 3D tensor operator, 
yielding a batch size number of vectors, to which all the b vector is added

We need to broadcast the b vector to all the 'column' vectors of the 2D tensor.
There is a formalism to organize these broadcasts:

https://data-apis.org/array-api/2023.12/API_specification/broadcasting.html

```
 A      (4d array):  8 x 1 x 6 x 1
 B      (3d array):      7 x 1 x 5
---------------------------------
 Result (4d array):  8 x 7 x 6 x 5
```

```
 A      (2d array):  5 x 4
 B      (1d array):      1
 -------------------------
 Result (2d array):  5 x 4
```

```
 A      (2d array):  5 x 4
 B      (1d array):      4
 -------------------------
 Result (2d array):  5 x 4
```

```
 A      (3d array):  15 x 3 x 5
 B      (3d array):  15 x 1 x 5
 ------------------------------
 Result (3d array):  15 x 3 x 5
```

```
 A      (3d array):  15 x 3 x 5
 B      (2d array):       3 x 5
 ------------------------------
 Result (3d array):  15 x 3 x 5
```

```
 A      (3d array):  15 x 3 x 5
 B      (2d array):       3 x 1
 ------------------------------
 Result (3d array):  15 x 3 x 5
```
