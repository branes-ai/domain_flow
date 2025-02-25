# Vertex Enumeration of a Convex Polytope

To enumerate the computational events in a computational domain, we need an algorithm to enumerate the vertices of a convex polytope defined by hyperplane constraints, with the goal of finding its encapsulating bounding box. A convex polytope in \( n \)-dimensional space can be represented as the intersection of a set of half-spaces, each defined by a linear inequality of the form \( a_i \cdot x \leq b_i \), where \( a_i \) is the normal vector of the \( i \)-th hyperplane, \( x \) is a point in \( \mathbb{R}^n \), and \( b_i \) is a scalar. The vertices of the polytope are the points where exactly \( n \) of these hyperplanes intersect (in a non-degenerate case), and the bounding box is the smallest axis-aligned box containing all these vertices.

To enumerate the vertices, we can use a well-established approach based on solving systems of linear equations derived from the hyperplane constraints. The idea is to select subsets of \( n \) inequalities, set them to equality (i.e., \( a_i \cdot x = b_i \)), and solve for \( x \), then check if the solution satisfies all remaining inequalities. This is a combinatorial problem, but it’s systematic and leverages the polytope’s H-representation (hyperplane constraints). Once we have the vertices, computing the bounding box is straightforward: find the minimum and maximum coordinates along each dimension.

Here’s the algorithm:

---

### Algorithm: Enumerate Vertices of a Convex Polytope and Compute Bounding Box

#### Input:
- A set of \( m \) hyperplane constraints defining the polytope in \( n \)-dimensional space:
  - For \( i = 1, 2, \ldots, m \): \( a_i \cdot x \leq b_i \), where \( a_i \in \mathbb{R}^n \) and \( b_i \in \mathbb{R} \).
- Assume the polytope is bounded (a polytope, not a polyhedron with unbounded rays) and non-empty.

#### Output:
- A list of vertex coordinates.
- The bounding box defined by \( [x_{\text{min},1}, x_{\text{max},1}] \times [x_{\text{min},2}, x_{\text{max},2}] \times \cdots \times [x_{\text{min},n}, x_{\text{max},n}] \).

#### Steps:
1. **Initialize:**
   - Let \( V \) be an empty list to store the vertices.
   - Define the constraint matrix \( A \) (size \( m \times n \)) where row \( i \) is \( a_i \).
   - Define the vector \( b \) (size \( m \)) where entry \( i \) is \( b_i \).

2. **Generate Vertex Candidates:**
   - For each combination \( S \) of \( n \) distinct indices from \( \{1, 2, \ldots, m\} \) (there are \( \binom{m}{n} \) such combinations):
     - Form the \( n \times n \) matrix \( A_S \) by selecting rows of \( A \) corresponding to indices in \( S \).
     - Form the \( n \)-vector \( b_S \) by selecting entries of \( b \) corresponding to indices in \( S \).
     - Solve the linear system \( A_S x = b_S \) for \( x \).
       - If \( A_S \) is singular (determinant is zero), skip to the next combination (the intersection is not a single point).
       - Otherwise, compute \( x = A_S^{-1} b_S \) (e.g., using Gaussian elimination or a numerical solver).

3. **Verify Feasibility:**
   - For each solution \( x \) from Step 2:
     - Check if \( x \) satisfies all \( m \) inequalities: \( A x \leq b \) (element-wise).
       - For \( i = 1, 2, \ldots, m \): ensure \( a_i \cdot x \leq b_i \) (with equality holding for \( i \in S \)).
     - If \( x \) satisfies all inequalities, add \( x \) to the list \( V \).

4. **Compute Bounding Box:**
   - If \( V \) is empty, the polytope may be degenerate or empty; terminate with an appropriate message.
   - For each dimension \( j = 1, 2, \ldots, n \):
     - \( x_{\text{min},j} = \min \{ x_j \mid x \in V \} \)
     - \( x_{\text{max},j} = \max \{ x_j \mid x \in V \} \)
   - Define the bounding box as the product of intervals \( [x_{\text{min},1}, x_{\text{max},1}] \times \cdots \times [x_{\text{min},n}, x_{\text{max},n}] \).

5. **Return:**
   - The list of vertices \( V \).
   - The bounding box coordinates.

---

### Pseudocode
```plaintext
function enumerateVerticesAndBoundingBox(A, b):
    m = number of rows in A
    n = number of columns in A
    V = empty list
    
    # Step 2: Generate all combinations of n hyperplanes
    for each combination S of n indices from {1, 2, ..., m}:
        A_S = submatrix of A with rows indexed by S
        b_S = subvector of b with entries indexed by S
        
        if det(A_S) != 0:
            x = solve(A_S * x = b_S)
            
            # Step 3: Check feasibility
            if A * x <= b (all inequalities hold):
                append x to V
    
    # Step 4: Compute bounding box
    if V is empty:
        return "Polytope is empty or degenerate"
    
    for j from 1 to n:
        x_min[j] = min(x[j] for x in V)
        x_max[j] = max(x[j] for x in V)
    
    bounding_box = [(x_min[j], x_max[j]) for j from 1 to n]
    return V, bounding_box
```

---

### Example
Suppose you have a 2D polytope defined by:
- \( x_1 \geq 0 \) (i.e., \( -x_1 \leq 0 \))
- \( x_2 \geq 0 \) (i.e., \( -x_2 \leq 0 \))
- \( x_1 + x_2 \leq 1 \)

Constraints:
- \( A = \begin{bmatrix} -1 & 0 \\ 0 & -1 \\ 1 & 1 \end{bmatrix} \), \( b = \begin{bmatrix} 0 \\ 0 \\ 1 \end{bmatrix} \)
- \( m = 3 \), \( n = 2 \).

Combinations of 2 indices:
1. \( \{1, 2\} \): \( -x_1 = 0 \), \( -x_2 = 0 \) → \( x_1 = 0 \), \( x_2 = 0 \) → \( (0, 0) \)
   - Check: \( 0 + 0 \leq 1 \) → Valid.
2. \( \{1, 3\} \): \( -x_1 = 0 \), \( x_1 + x_2 = 1 \) → \( x_1 = 0 \), \( x_2 = 1 \) → \( (0, 1) \)
   - Check: \( 0 \leq 0 \), \( 1 \leq 1 \) → Valid.
3. \( \{2, 3\} \): \( -x_2 = 0 \), \( x_1 + x_2 = 1 \) → \( x_2 = 0 \), \( x_1 = 1 \) → \( (1, 0) \)
   - Check: \( 1 \leq 0 \), \( 0 \leq 1 \) → Valid.

Vertices: \( V = \{(0, 0), (0, 1), (1, 0)\} \).

Bounding box:
- \( x_1 \): min = 0, max = 1 → \( [0, 1] \)
- \( x_2 \): min = 0, max = 1 → \( [0, 1] \)
- Result: \( [0, 1] \times [0, 1] \).

---

### Notes
- **Efficiency:** For large \( m \) and \( n \), \( \binom{m}{n} \) grows rapidly, making this brute-force approach computationally expensive. Practical implementations (e.g., in computational geometry libraries like `CGAL` or `pypoman`) use more sophisticated methods like vertex enumeration via dual representation or incremental algorithms.
- **Degeneracy:** If more than \( n \) hyperplanes intersect at a point, additional care is needed to avoid duplicates (e.g., tracking unique solutions).
- **Boundedness:** If the polytope is unbounded, this method fails; you’d need a V-representation or ray-tracing approach instead.

This algorithm gives you a complete list of vertices and the bounding box, perfect for index point enumeration or further geometric analysis! 