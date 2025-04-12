# The Woodbury Formula for low rank updates

The Woodbury matrix identity (often referred to as the Woodbury formula) is a powerful tool in linear algebra that allows you to efficiently compute the inverse of a matrix that has been modified by a low-rank update. Here's a breakdown of what it is and how it's used:

**Understanding the Woodbury Formula**

* **The Problem:**
    * In many applications, you might have a matrix whose inverse you already know.
    * Then, that matrix undergoes a change, specifically a "low-rank update." This means a relatively simple modification to the matrix.
    * Directly inverting the modified matrix can be computationally expensive, especially for large matrices.
* **The Solution:**
    * The Woodbury formula provides a way to calculate the inverse of the modified matrix using the original matrix's inverse, thus potentially saving significant computational effort.

**Mathematical Representation**

The formula is generally expressed as:

* $$(A + UCV)^{-1} = A^{-1} - A^{-1}U(C^{-1} + VA^{-1}U)^{-1}VA^{-1}$$

Where:

* A is the original invertible matrix.
* U and V are matrices that define the low-rank update.
* C is an invertible matrix.

**Why It's Efficient**

The efficiency of the Woodbury formula comes from the fact that:

* It replaces the inversion of the potentially large matrix (A + UCV) with the inversion of the usually much smaller matrix (C⁻¹ + VA⁻¹U).
* This can lead to significant computational savings, especially when U and V have a small number of columns.

**Applications**

The Woodbury formula finds applications in various fields, including:

* **Kalman Filtering:**
    * It's used extensively in Kalman filtering, a technique for estimating the state of a system based on noisy measurements.
* **Machine Learning:**
    * It can be useful in certain machine learning algorithms where matrices are updated iteratively.
* **Numerical Linear Algebra:**
    * It helps in optimizing matrix computations in various numerical applications.

**Key Considerations**

* The efficiency of the Woodbury formula depends heavily on the rank of the update (determined by the dimensions of U and V). The lower the rank, the greater the potential savings.
* The Sherman-Morrison formula is a special case of the Woodbury formula, when C is a 1x1 matrix.

In essence, the Woodbury formula is a valuable tool for efficiently updating matrix inverses, particularly when dealing with low-rank modifications.
