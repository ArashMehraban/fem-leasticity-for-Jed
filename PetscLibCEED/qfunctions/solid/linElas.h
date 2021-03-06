#ifndef __LIN_ELAS__H
#define __LIN_ELAS__H

#ifndef PHYSICS_STRUCT
#define PHYSICS_STRUCT
typedef struct Physics_private *Physics;
struct Physics_private {
  PetscScalar   nu;      // Poisson's ratio
  PetscScalar   E;       // Young's Modulus
};
#endif

// -----------------------------------------------------------------------------
CEED_QFUNCTION(LinElasF)(void *ctx, CeedInt Q, const CeedScalar *const *in,
                         CeedScalar *const *out) {
  // *INDENT-OFF*
  // Inputs
  const CeedScalar (*ug)[3][Q] = (CeedScalar(*)[3][Q])in[0],
                   (*qdata)[Q] = (CeedScalar(*)[Q])in[1];

  // Outputs
  CeedScalar (*dvdX)[3][Q] = (CeedScalar(*)[3][Q])out[0];
             // gradu not used for linear elasticity
             // (*gradu)[3][Q] = (CeedScalar(*)[3][Q])out[1];
  // *INDENT-ON*

  // Context
  const Physics context = ctx;
  const CeedScalar E  = context->E;
  const CeedScalar nu = context->nu;

  // Quadrature Point Loop
  CeedPragmaSIMD
  for (CeedInt i=0; i<Q; i++) {
    // Read spatial derivatives of u
    // *INDENT-OFF*
    const CeedScalar du[3][3]   = {{ug[0][0][i],
                                    ug[1][0][i],
                                    ug[2][0][i]},
                                   {ug[0][1][i],
                                    ug[1][1][i],
                                    ug[2][1][i]},
                                   {ug[0][2][i],
                                    ug[1][2][i],
                                    ug[2][2][i]}
                                  };
    // -- Qdata
    const CeedScalar wJ         =    qdata[0][i];
    const CeedScalar dXdx[3][3] =  {{qdata[1][i],
                                     qdata[2][i],
                                     qdata[3][i]},
                                    {qdata[4][i],
                                     qdata[5][i],
                                     qdata[6][i]},
                                    {qdata[7][i],
                                     qdata[8][i],
                                     qdata[9][i]}
                                   };
    // *INDENT-ON*

    // Compute gradu
    // dXdx = (dx/dX)^(-1)
    // Apply dXdx to du = gradu
    CeedScalar gradu[3][3];
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        gradu[j][k] = 0;
        for (int m=0; m<3; m++)
          gradu[j][k] += dXdx[m][k] * du[j][m];
      }
printf("Qpt %d\n", i);
for (int j=0; j<3; j++) {
for (int k=0; k<3; k++) {
printf("%f\t",gradu[j][k]);
}
printf("\n");
}
    // Compute Strain : e (epsilon)
    // e = 1/2 (grad u + (grad u)^T)

    // *INDENT-OFF*
    const CeedScalar e[3][3] =  {{(gradu[0][0] + gradu[0][0])*0.5,
                                  (gradu[0][1] + gradu[1][0])*0.5,
                                  (gradu[0][2] + gradu[2][0])*0.5},
                                 {(gradu[1][0] + gradu[0][1])*0.5,
                                  (gradu[1][1] + gradu[1][1])*0.5,
                                  (gradu[1][2] + gradu[2][1])*0.5},
                                 {(gradu[2][0] + gradu[0][2])*0.5,
                                  (gradu[2][1] + gradu[1][2])*0.5,
                                  (gradu[2][2] + gradu[2][2])*0.5}
                                };
    // *INDENT-ON*

    //
    // Formulation uses Voigt notation:
    //  stress (sigma)      strain (epsilon)
    //
    //    [sigma00]             [e00]
    //    [sigma11]             [e11]
    //    [sigma22]   =  S   *  [e22]
    //    [sigma12]             [e12]
    //    [sigma02]             [e02]
    //    [sigma01]             [e01]
    //
    //        where
    //                         [1-nu   nu    nu                                    ]
    //                         [ nu   1-nu   nu                                    ]
    //                         [ nu    nu   1-nu                                   ]
    // S = E/((1+nu)*(1-2*nu)) [                  (1-2*nu)/2                       ]
    //                         [                             (1-2*nu)/2            ]
    //                         [                                        (1-2*nu)/2 ]

    //Above Voigt Notation is placed in a 3x3 matrix:
    const CeedScalar ss      =  E/((1+nu)*(1-2*nu));
    const CeedScalar sigma00 = ss*((1-nu)*e[0][0] + nu*e[1][1] +nu*e[2][2]),
                     sigma11 = ss*(nu*e[0][0] + (1-nu)*e[1][1] +nu*e[2][2]),
                     sigma22 = ss*(nu*e[0][0] + nu*e[1][1] +(1-nu)*e[2][2]),
                     sigma12 = ss*(1-2*nu)*e[1][2]*0.5,
                     sigma02 = ss*(1-2*nu)*e[0][2]*0.5,
                     sigma01 = ss*(1-2*nu)*e[0][1]*0.5;
    const CeedScalar sigma[3][3] = {
        {sigma00, sigma01, sigma02},
        {sigma01, sigma11, sigma12},
        {sigma02, sigma12, sigma22}
       };

    // Apply dXdx^T and weight to sigma
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        dvdX[k][j][i] = 0;
        for (int m=0; m<3; m++)
          dvdX[k][j][i] += dXdx[k][m] * sigma[j][m] * wJ;
      }
    } // End of Quadrature Point Loop

  return 0;
}

// -----------------------------------------------------------------------------
CEED_QFUNCTION(LinElasdF)(void *ctx, CeedInt Q, const CeedScalar *const *in,
                          CeedScalar *const *out) {
  // *INDENT-OFF*
  // Inputs
  const CeedScalar (*deltaug)[3][Q] = (CeedScalar(*)[3][Q])in[0],
                   (*qdata)[Q] = (CeedScalar(*)[Q])in[1];
                   // gradu not used for linear elasticity
                   // (*gradu)[3][Q] = (CeedScalar(*)[3][Q])in[2];


  // Outputs
  CeedScalar (*deltadvdX)[3][Q] = (CeedScalar(*)[3][Q])out[0];
  // *INDENT-ON*

  // Context
  const Physics context = ctx;
  const CeedScalar E  = context->E;
  const CeedScalar nu = context->nu;

  // Quadrature Point Loop
  CeedPragmaSIMD
  for (CeedInt i=0; i<Q; i++) {
    // Read spatial derivatives of u
    // *INDENT-OFF*
    const CeedScalar deltadu[3][3] = {{deltaug[0][0][i],
                                       deltaug[1][0][i],
                                       deltaug[2][0][i]},
                                      {deltaug[0][1][i],
                                       deltaug[1][1][i],
                                       deltaug[2][1][i]},
                                      {deltaug[0][2][i],
                                       deltaug[1][2][i],
                                       deltaug[2][2][i]}
                                     };
    // -- Qdata
    const CeedScalar wJ         =    qdata[0][i];
    const CeedScalar dXdx[3][3] =  {{qdata[1][i],
                                     qdata[2][i],
                                     qdata[3][i]},
                                    {qdata[4][i],
                                     qdata[5][i],
                                     qdata[6][i]},
                                    {qdata[7][i],
                                     qdata[8][i],
                                     qdata[9][i]}
                                   };
    // *INDENT-ON*

    //Compute graddeltau
    //dXdx = (dx/dX)^(-1)
    // Apply dXdx to deltadu = graddeltau
    CeedScalar graddeltau[3][3];
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        graddeltau[j][k] = 0;
        for (int m=0; m<3; m++)
          graddeltau[j][k] += dXdx[m][k] * deltadu[j][m];
      }

    // Compute Strain : e (epsilon)
    // e = 1/2 (grad u + (grad u)^T)
    // *INDENT-OFF*
    const CeedScalar de[3][3]     =  {{(graddeltau[0][0] + graddeltau[0][0])*0.5,
                                       (graddeltau[0][1] + graddeltau[1][0])*0.5,
                                       (graddeltau[0][2] + graddeltau[2][0])*0.5},
                                      {(graddeltau[1][0] + graddeltau[0][1])*0.5,
                                       (graddeltau[1][1] + graddeltau[1][1])*0.5,
                                       (graddeltau[1][2] + graddeltau[2][1])*0.5},
                                      {(graddeltau[2][0] + graddeltau[0][2])*0.5,
                                       (graddeltau[2][1] + graddeltau[1][2])*0.5,
                                       (graddeltau[2][2] + graddeltau[2][2])*0.5}
                                     };

    // *INDENT-ON*
    // Formulation uses Voigt notation:
    //  stress (sigma)      strain (epsilon)
    //
    //    [dsigma00]             [de00]
    //    [dsigma11]             [de11]
    //    [dsigma22]   =  S   *  [de22]
    //    [dsigma12]             [de12]
    //    [dsigma02]             [de02]
    //    [dsigma01]             [de01]
    //
    //        where
    //                         [1-nu   nu    nu                                    ]
    //                         [ nu   1-nu   nu                                    ]
    //                         [ nu    nu   1-nu                                   ]
    // S = E/((1+nu)*(1-2*nu)) [                  (1-2*nu)/2                       ]
    //                         [                             (1-2*nu)/2            ]
    //                         [                                        (1-2*nu)/2 ]

    //Above Voigt Notation is placed in a 3x3 matrix:
    const CeedScalar ss      =  E/((1+nu)*(1-2*nu));
    const CeedScalar dsigma00 = ss*((1-nu)*de[0][0] + nu*de[1][1] +nu*de[2][2]),
                     dsigma11 = ss*(nu*de[0][0] + (1-nu)*de[1][1] +nu*de[2][2]),
                     dsigma22 = ss*(nu*de[0][0] + nu*de[1][1] +(1-nu)*de[2][2]),
                     dsigma12 = ss*(1-2*nu)*de[1][2]/2,
                     dsigma02 = ss*(1-2*nu)*de[0][2]/2,
                     dsigma01 = ss*(1-2*nu)*de[0][1]/2;
    const CeedScalar dsigma[3][3] =
        { {dsigma00, dsigma01, dsigma02},
          {dsigma01, dsigma11, dsigma12},
          {dsigma02, dsigma12, dsigma22}
        };

    // Apply dXdx^T and weight
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        deltadvdX[k][j][i] = 0;
        for (int m=0; m<3; m++)
          deltadvdX[k][j][i] += dXdx[k][m] * dsigma[j][m] * wJ;
      }
    } // End of Quadrature Point Loop

   return 0;
}

#endif //End of __LIN_ELAS__H
