// *****************************************************************************
// This QFunction sets up the geometric factors required for integration and
//   coordinate transformations
//
// Reference (parent) coordinates: X
// Physical (current) coordinates: x
// Change of coordinate matrix: dxdX_{i,j} = x_{i,j} (indicial notation)
// Inverse of change of coordinate matrix: dXdx_{i,j} = (detJ^-1) * X_{i,j}
//
// All quadrature data is stored in 10 field vector of quadrature data.
//
// We require the transpose of the inverse of the Jacobian to properly compute
//   integrals of the form: int( gradv u )
//
// Inverse of Jacobian:
//   dXdx_i,j = Aij / detJ
//
// Stored: Aij / detJ
//   in qdata[1:9] as
//              [A11 A12 A13]
//  (detJ^-1) * [A21 A22 A23]
//              [A31 A32 A33]
//
// *****************************************************************************
CEED_QFUNCTION(SetupGeo)(void *ctx, CeedInt Q, const CeedScalar *const *in,
                         CeedScalar *const *out) {

    // *INDENT-OFF*
     // Inputs
     const CeedScalar (*J)[3][Q] = (CeedScalar(*)[3][Q])in[0],
                      (*w) = in[1];

     // Outputs
     CeedScalar (*qdata)[Q] = (CeedScalar(*)[Q])out[0];
     // *INDENT-ON*

  CeedPragmaSIMD
  // Quadrature Point Loop
  for (CeedInt i=0; i<Q; i++) {
    // Setup
    const CeedScalar J11 = J[0][0][i];
    const CeedScalar J21 = J[0][1][i];
    const CeedScalar J31 = J[0][2][i];
    const CeedScalar J12 = J[1][0][i];
    const CeedScalar J22 = J[1][1][i];
    const CeedScalar J32 = J[1][2][i];
    const CeedScalar J13 = J[2][0][i];
    const CeedScalar J23 = J[2][1][i];
    const CeedScalar J33 = J[2][2][i];
    const CeedScalar A11 = J22*J33 - J23*J32;
    const CeedScalar A12 = J13*J32 - J12*J33;
    const CeedScalar A13 = J12*J23 - J13*J22;
    const CeedScalar A21 = J23*J31 - J21*J33;
    const CeedScalar A22 = J11*J33 - J13*J31;
    const CeedScalar A23 = J13*J21 - J11*J23;
    const CeedScalar A31 = J21*J32 - J22*J31;
    const CeedScalar A32 = J12*J31 - J11*J32;
    const CeedScalar A33 = J11*J22 - J12*J21;
    const CeedScalar detJ = J11*A11 + J21*A12 + J31*A13;

    // Qdata
    // -- Interp-to-Interp qdata
    qdata[0][i] = w[i] * detJ;
    // -- Interp-to-Grad qdata
    // Inverse of change of coordinate matrix: X_i,j
    qdata[1][i] = A11 / detJ;
    qdata[2][i] = A12 / detJ;
    qdata[3][i] = A13 / detJ;
    qdata[4][i] = A21 / detJ;
    qdata[5][i] = A22 / detJ;
    qdata[6][i] = A23 / detJ;
    qdata[7][i] = A31 / detJ;
    qdata[8][i] = A32 / detJ;
    qdata[9][i] = A33 / detJ;

  } // End of Quadrature Point Loop

  return 0;
}
// -----------------------------------------------------------------------------
