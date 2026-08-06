# Mechanical constitutive-law audit and test additions

## Scope

This document describes the audit of `MechanicalOperator` and
`MechanicalPhysics`, the defects found in the constitutive stress update, and
the unit and integration tests added to prevent regressions. The implementation
is compared with Adamantine's documented small-strain thermoelastoplastic model
and radial-return algorithm.

The work is divided into three code/test patches:

1. `9ccfa6f` (`Test mechanical operator weak forms`)
2. `5ac2260` (`Fix mechanical constitutive stress history`)
3. `a50dc94` (`Test thermomechanical linearity`)

No production defect was found in `MechanicalOperator`. Its elastic bilinear
form, gravity load, thermal load sign, and reference-temperature selection are
now covered by exact weak-form tests. Two defects were found and fixed in
`MechanicalPhysics`.

## Governing equations used in the audit

For displacement $\boldsymbol{u}$, the infinitesimal strain is

$$
\boldsymbol{\varepsilon}
= \frac{1}{2}\left(\nabla\boldsymbol{u}
+ \nabla\boldsymbol{u}^{T}\right).
$$

The isotropic thermoelastic stress is

$$
\boldsymbol{\sigma}
= \lambda\operatorname{tr}(\boldsymbol{\varepsilon})\boldsymbol{I}
+ 2\mu\boldsymbol{\varepsilon}
- \beta(T-T_{\mathrm{ref}})\boldsymbol{I},
\qquad
\beta=(3\lambda+2\mu)\alpha.
$$

For the combined isotropic-kinematic hardening model, Adamantine defines

$$
\boldsymbol{\xi}^{\mathrm{trial}}
= \boldsymbol{s}^{\mathrm{trial}}-\boldsymbol{\gamma}_n,
\qquad
\chi=\left\|\boldsymbol{\xi}^{\mathrm{trial}}\right\|,
$$

where $\boldsymbol{s}^{\mathrm{trial}}$ is the deviatoric trial stress,
$\boldsymbol{\gamma}_n$ is the back stress, and $\kappa_n$ is the current
isotropic yield radius. The step is elastic when

$$
\chi \leq \kappa_n.
$$

Otherwise, with plastic modulus $H$ and isotropic-hardening fraction $a$,

$$
\Delta\eta=\frac{\chi-\kappa_n}{2\mu+H},
\qquad
\boldsymbol{n}=\frac{\boldsymbol{\xi}^{\mathrm{trial}}}{\chi},
$$

and the return update is

$$
\begin{aligned}
\boldsymbol{\sigma}_{n+1}
  &=\boldsymbol{\sigma}^{\mathrm{trial}}
    -2\mu\Delta\eta\boldsymbol{n},\\
\kappa_{n+1}&=\kappa_n+aH\Delta\eta,\\
\boldsymbol{\gamma}_{n+1}
  &=\boldsymbol{\gamma}_n+(1-a)H\Delta\eta\boldsymbol{n}.
\end{aligned}
$$

## Bug 1: equality at the yield surface entered the plastic branch

### Previous behavior

`MechanicalPhysics::compute_stress()` used

```cpp
if (effective_stress_norm < plastic_internal_variable)
```

even though the documented elastic condition is $\chi\leq\kappa$. This is not
only a convention difference. For a stress-free material whose initial elastic
limit is zero, $\chi=\kappa=0$. The strict comparison sent that state to the
plastic branch, which formed

$$
\boldsymbol{n}=\frac{\boldsymbol{0}}{0}.
$$

Although the computed plastic multiplier is also zero, subsequent expressions
contain zero multiplied by the invalid direction. The stored stress and back
stress can therefore become NaN on the first zero-load solve.

### Fix

The branch now uses `<=`, exactly matching the governing equations. The flow
direction is only evaluated when $\chi>\kappa$, so division by zero is avoided
without a numerical tolerance or an arbitrary perturbation.

### Regression test

`elastoplastic_radial_return` first solves a completely clamped, unloaded cube
with zero initial elastic limit. It verifies that every stored stress component
is finite and zero.

The test then applies a trace-free trial stress

$$
\boldsymbol{\sigma}^{\mathrm{trial}}
=\operatorname{diag}(1,-1,0),
$$

with $\mu=3$, $H=1.5$, and $a=0.25$. Its norm is $\sqrt{2}$ and the exact first
plastic increment is

$$
\Delta\eta_1=\frac{\sqrt{2}}{2(3)+1.5}.
$$

Every cell and quadrature point is compared with the exact radial return. A
second collinear stress increment of magnitude $0.2$ is then applied. Because
the first step has nonzero isotropic and kinematic history, this second return
also validates preservation and reuse of $\kappa$ and $\boldsymbol{\gamma}$.
In particular, it protects the back-stress formula fixed immediately before
this work in commit `51f08dc`: the update must contain $H\Delta\eta$, not
$H^2$ independent of the plastic increment.

## Bug 2: stored stress omitted thermal eigenstress

### Previous behavior

`MechanicalOperator` correctly assembled the thermal load in the displacement
equation, but `MechanicalPhysics::compute_stress()` updated the stored stress
using only the displacement-strain increment:

$$
\boldsymbol{\sigma}_{n+1}
=\boldsymbol{\sigma}_n+\mathbb{C}:\Delta\boldsymbol{\varepsilon}.
$$

The term $-\beta(T-T_{\mathrm{ref}})\boldsymbol{I}$ was absent. A uniformly
heated cube constrained on every boundary has $\boldsymbol{u}=0$. The old
postprocessing therefore stored zero stress, whereas the analytical solution is

$$
\boldsymbol{\sigma}
=-\beta(T-T_{\mathrm{ref}})\boldsymbol{I}.
$$

This made mechanical stress output incorrect and left the constitutive history
inconsistent with the thermoelastic problem solved by `MechanicalOperator`.

### Fix

`MechanicalPhysics` now stores the previous hydrostatic thermal-stress scalar
at every cell quadrature point:

$$
b_n=\beta_n(T_n-T_{\mathrm{ref},n}).
$$

The trial stress update is

$$
\boldsymbol{\sigma}^{\mathrm{trial}}_{n+1}
=\boldsymbol{\sigma}_n
+\mathbb{C}:\Delta\boldsymbol{\varepsilon}
-(b_{n+1}-b_n)\boldsymbol{I}.
$$

Using the increment $b_{n+1}-b_n$ is important: adding the total thermal stress
on every mechanical solve would count previous heating repeatedly. It also
handles changes in material properties and changes from the initial substrate
reference temperature to the material-specific reference temperature after a
cell melts.

The thermal history is initialized for new cells, retained for unchanged
cells, and included in `CellDataTransfer` packing and unpacking so mesh
adaptation does not lose or duplicate thermal stress.

### Analytical regression test

`thermoelastic_stress_uniform_heating` uses a unit cube clamped on all six
faces, with $\lambda=2$, $\mu=3$, $\alpha=0.01$, and
$T_{\mathrm{ref}}=300$. At $T=350$,

$$
\beta=(3(2)+2(3))(0.01)=0.12,
\qquad
\boldsymbol{\sigma}=-6\boldsymbol{I}.
$$

The test verifies zero displacement and this stress at every quadrature point.
It then raises the temperature to $360$, solves again, and verifies
$\boldsymbol{\sigma}=-7.2\boldsymbol{I}$. The second check would fail if the
total thermal stress were added twice instead of incrementally.

## Patch 1: exact `MechanicalOperator` weak-form tests (`9ccfa6f`)

This patch changes only `tests/test_mechanical_operator.cc`.

- The affine field $\boldsymbol{u}=(x,0,0)$ verifies the exact elastic energy
  $a(\boldsymbol{u},\boldsymbol{u})=(\lambda+2\mu)|\Omega|$.
- The virtual field $\boldsymbol{v}=(0,0,x)$ verifies the exact gravity work
  $-\rho g|\Omega|L/2$ on the rectangular domain.
- The same affine virtual field verifies the thermal weak form
  $\int_\Omega\beta(T-T_{\mathrm{ref}})\nabla\cdot\boldsymbol{v}\,d\Omega$.
- Switching `has_melted` from false to true verifies selection of the initial
  substrate reference temperature and then the material reference temperature,
  including the expected sign reversal in the chosen test data.

These checks exercise matrix action and right-hand-side assembly directly,
without relying on a numerical displacement solution or stored golden vector.

## Patch 2: constitutive stress fixes and tests (`5ac2260`)

This patch changes `source/MechanicalPhysics.cc`,
`source/MechanicalPhysics.hh`, and `tests/test_mechanical_physics.cc`.

- It fixes the elastic/plastic boundary comparison.
- It carries temperature, melting state, and reference-temperature information
  into stress recovery.
- It adds incremental thermal stress to the trial stress.
- It preserves thermal stress through cell activation and distributed mesh
  transfer.
- It adds the two-step radial-return and constrained-heating tests.
- It documents the existing Eshelby inclusion benchmark with its analytical
  displacement form and primary reference.

## Patch 3: thermomechanical integration test (`a50dc94`)

This patch changes only `tests/test_integration_thermoelastic.cc`.

For linear thermoelasticity with temperature-independent thermal properties,
changing $\alpha$ does not affect the heat equation, while displacement is
linear in $\alpha$. The integration test runs the complete application twice
and checks

$$
T_{2\alpha}=T_{\alpha},
\qquad
\boldsymbol{u}_{2\alpha}=2\boldsymbol{u}_{\alpha}.
$$

Large test-only expansion coefficients raise the displacement safely above
roundoff; the asserted linear relation is unchanged. The case uses a small
mesh and short duration, and it passes with one and two MPI ranks.

## Validation

The changes were formatted with the repository's configured `clang-format`
style, limited to the files in these patches so unrelated working-tree changes
were not modified.

The following build and tests passed in the Nix development environment:

```console
nix develop /home/cloud/adamantine --command ninja \
  test_mechanical_operator test_mechanical_physics \
  test_integration_thermoelastic

nix develop /home/cloud/adamantine --command ctest --output-on-failure \
  -R 'mechanical_(operator|physics)'

# Run from build/bin, where CMake copies the integration-test inputs.
nix develop /home/cloud/adamantine --command \
  ./test_integration_thermoelastic \
  --run_test=integration_thermomechanical_linearity

nix develop /home/cloud/adamantine --command mpiexec -n 2 \
  ./test_integration_thermoelastic \
  --run_test=integration_thermomechanical_linearity
```

The complete one-rank pre-existing thermoelastic integration regression also
passed. The focused new integration case passed in both one-rank and two-rank
configurations.

## References

1. Adamantine,
   [Governing equations: elastoplasticity](https://adamantine-sim.github.io/adamantine/doc/governing_equations.html#elastoplasticity).
2. Ronaldo I. Borja, *Plasticity: Modeling & Computation*, Springer, 2013,
   Chapter 3,
   [doi:10.1007/978-3-642-38547-6](https://doi.org/10.1007/978-3-642-38547-6).
3. J. D. Eshelby, “The Determination of the Elastic Field of an Ellipsoidal
   Inclusion, and Related Problems,” *Proceedings of the Royal Society A*, 241
   (1957), 376–396,
   [doi:10.1098/rspa.1957.0133](https://doi.org/10.1098/rspa.1957.0133).
4. Y. C. Fung and Pin Tong, *Classical and Computational Solid Mechanics*,
   World Scientific, 2001, Chapter 14,
   [doi:10.1142/4362](https://doi.org/10.1142/4362).
