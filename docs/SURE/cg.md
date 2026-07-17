# Conjugate Gradient — a coupled domain-flow graph

Conjugate Gradient solves an SPD system `Ax = b` by building, at each step, a search
direction `A`-conjugate to all the previous ones — and it converges *exactly* in at most
`N` steps. It is the most tightly **coupled** entry in the catalog: one CG iteration is a
small domain-flow graph of L1/L2 kernels, stitched together by two scalars that feed back
across the iteration index `k`.

## One iteration, five kernels, two scalars

$$
\begin{aligned}
Ap^{k} &= A\,p^{k} && \text{(SpMV / gemv)}\\
\alpha^{k} &= \frac{r^{k}\!\cdot r^{k}}{p^{k}\!\cdot Ap^{k}} && \text{(two dots)}\\
x^{k+1} &= x^{k} + \alpha^{k} p^{k}, \qquad r^{k+1} = r^{k} - \alpha^{k} Ap^{k} && \text{(two axpys)}\\
\beta^{k} &= \frac{r^{k+1}\!\cdot r^{k+1}}{r^{k}\!\cdot r^{k}}, \qquad p^{k+1} = r^{k+1} + \beta^{k} p^{k} && \text{(dot, axpy)}
\end{aligned}
$$

The residual/search-direction chain is the whole story: `r` drives `α`, `α` updates `x`
and `r`, the new `r` drives `β`, and `β` bends `p` for the next step. Nothing here is a
new kernel — it is `gemv`, `dot`, and `axpy` from the catalog, wired into a cycle.

## Two reduction directions in one index space

The index space `(i, j, k)` — component `i`, SpMV column `j`, iteration `k` — carries
**two reduction directions at once**:

- the **SpMV** `Ap_i = Σ_j A_{ij} p_j` accumulates along `+j` (a `gemv`, reading `p_j`
  across rows as an affine gather);
- the two **dot products** `p·Ap` and `r·r` accumulate along `+i`, over the components,
  each finishing at `(N-1, N-1, k)`.

Those dot results are scalars, and `α^k`, `β^k` are their ratios — **broadcast back to
every component** for the axpys. Reading a scalar at `(N-1,N-1,k)` from every `(i,j)` is
an affine tap, as is the `p_j` gather, so CG is a **SARE** — eleven affine arcs in its
RDG, the most of any operator in the catalog.

```text
ap(i,j,k)   = ap(i,j-1,k) + am(i,j,k) * pv(j,N-1,k);      // SpMV, reduce +j
pApd(i,j,k) = pApd(i-1,j,k) + pv(i,N-1,k)*ap(i,N-1,k);    // dot p.Ap, reduce +i
rrd(i,j,k)  = rrd(i-1,j,k) + rv(i,N-1,k)*rv(i,N-1,k);     // dot r.r,  reduce +i
alp(i,j,k)  = rrd(N-1,N-1,k) / pApd(N-1,N-1,k);           // alpha^k  (broadcast)
xv(i,j,k)   = xv(i,j,k-1) + alp(i,j,k-1)*pv(i,N-1,k-1);   // x^k axpy
rv(i,j,k)   = rv(i,j,k-1) - alp(i,j,k-1)*ap(i,N-1,k-1);   // r^k axpy
pv(i,j,k)   = rv(i,j,k)   + bet(i,j,k)  *pv(i,N-1,k-1);   // p^k axpy
```

Executable spec: `docs/SURE/cg.sure` (`am` holds `A`; the `k=-1` halo seeds `x^0=0`,
`r^0=p^0=b` with `α^{-1}=0` so the `k=0` cell is the initial state). For the bundled SPD
tridiagonal `A` and `b = [1,2,3]`, four steps reach the exact solution
`x = [0.1786, 0.2857, 0.6786]` — verified by `test_cg_sure` to machine precision.

## Schedule

CG is **free-schedule only**: the scalar `α`/`β` depend on reductions that complete at
`(N-1,N-1,k)` and then feed the same iteration's updates, and `p^k = r^k + …` reads `r^k`
at the same cell — zero-slack couplings no single linear `τ` can thread. The free
schedule runs the SpMV, the two dot reductions, the scalar evaluations, and the three
axpys in exactly their data-flow order, one iteration after another — the coupled graph
made explicit.

## The RDG

The densest graph in the catalog: nine variables (the held `A`, the SpMV accumulator, the
two dot accumulators, the two scalars, and the three vectors) wired by eleven **affine**
arcs — the SpMV gather, the dot reads of the finished vectors, and the scalar broadcasts —
plus the uniform reduction chains and the `k-1` iteration carries.

<div class="rdg" data-src="rdg/cg.json" data-height="680"></div>

<div class="schedule-anim" data-src="schedules/cg-free.json" data-height="380"></div>
