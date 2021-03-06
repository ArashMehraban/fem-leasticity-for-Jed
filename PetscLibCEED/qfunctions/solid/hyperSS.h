#ifndef __HYPER_SS__H
#define __HYPER_SS__H

#include<math.h>

#ifndef PHYSICS_STRUCT
#define PHYSICS_STRUCT
typedef struct Physics_private *Physics;
struct Physics_private {
  PetscScalar   nu;      // Poisson's ratio
  PetscScalar   E;       // Young's Modulus
};
#endif

// -----------------------------------------------------------------------------
CEED_QFUNCTION(HyperSSF)(void *ctx, CeedInt Q, const CeedScalar *const *in,
                         CeedScalar *const *out) {
  // *INDENT-OFF*
  // Inputs
  const CeedScalar (*ug)[3][Q] = (CeedScalar(*)[3][Q])in[0],
                   (*qdata)[Q] = (CeedScalar(*)[Q])in[1];

  // Outputs
  CeedScalar (*dvdX)[3][Q] = (CeedScalar(*)[3][Q])out[0];
  // gradu is used for hyperelasticity small strain (non-linear)
  CeedScalar (*gradu)[3][Q] = (CeedScalar(*)[3][Q])out[1];
  // *INDENT-ON*

  // Context
  const Physics context = ctx;
  const CeedScalar E  = context->E;
  const CeedScalar nu = context->nu;
  const CeedScalar TwoMu = E/(1+nu);
  const CeedScalar Kbulk = E/(3*(1-2*nu)); //bulk Modulus
  const CeedScalar lambda = (3*Kbulk-TwoMu)/3;

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
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        gradu[j][k][i] = 0;
        for (int m=0; m<3; m++)
          gradu[j][k][i] += dXdx[m][k] * du[j][m];
      }

    // Compute Strain : e (epsilon)
    // e = 1/2 (grad u + (grad u)^T)
     // *INDENT-OFF*
     const CeedScalar e[3][3] =  {{(gradu[0][0][i] + gradu[0][0][i])*0.5,
                                   (gradu[0][1][i] + gradu[1][0][i])*0.5,
                                   (gradu[0][2][i] + gradu[2][0][i])*0.5},
                                  {(gradu[1][0][i] + gradu[0][1][i])*0.5,
                                   (gradu[1][1][i] + gradu[1][1][i])*0.5,
                                   (gradu[1][2][i] + gradu[2][1][i])*0.5},
                                  {(gradu[2][0][i] + gradu[0][2][i])*0.5,
                                   (gradu[2][1][i] + gradu[1][2][i])*0.5,
                                   (gradu[2][2][i] + gradu[2][2][i])*0.5}
                                 };

    // *INDENT-ON*
    //strain (epsilon)
    //    and
    //stress (sigma) in Voigt notation:
    //           [e11]              [sigma11]
    //           [e22]              [sigma22]
    // epsilon = [e33]  ,   sigma = [sigma33]
    //           [e23]              [sigma23]
    //           [e13]              [sigma13]
    //           [e12]              [sigma12]
    //
    // Sigma = S * epsilon
    //                         [1-nu   nu    nu                                    ]
    //                         [ nu   1-nu   nu                                    ]
    //                         [ nu    nu   1-nu                                   ]
    // S = E/((1+nu)*(1-2*nu)) [                  (1-2*nu)/2                       ]
    //                         [                             (1-2*nu)/2            ]
    //                         [                                        (1-2*nu)/2 ]





    //Above Voigt Notation is placed in a 3x3 matrix:
    //Volumetric Strain
    const CeedScalar strain_vol = e[0][0] + e[1][1] + e[2][2];
    const CeedScalar sigma00 = lambda*log(1+strain_vol) + TwoMu*e[0][0],
                     sigma11 = lambda*log(1+strain_vol) + TwoMu*e[1][1],
                     sigma22 = lambda*log(1+strain_vol) + TwoMu*e[2][2],
                     sigma12 = TwoMu*e[1][2],
                     sigma02 = TwoMu*e[0][2],
                     sigma01 = TwoMu*e[0][1];
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
CEED_QFUNCTION(HyperSSdF)(void *ctx, CeedInt Q, const CeedScalar *const *in,
                          CeedScalar *const *out) {
  // *INDENT-OFF*
  // Inputs
  const CeedScalar (*deltaug)[3][Q] = (CeedScalar(*)[3][Q])in[0],
                   (*qdata)[Q] = (CeedScalar(*)[Q])in[1];
  // gradu is used for hyperelasticity (non-linear)
  const CeedScalar (*gradu)[3][Q] = (CeedScalar(*)[3][Q])in[2];

  // Outputs
  CeedScalar (*deltadvdX)[3][Q] = (CeedScalar(*)[3][Q])out[0];
  // *INDENT-ON*

  // Context
  const Physics context = ctx;
  const CeedScalar E  = context->E;
  const CeedScalar nu = context->nu;
  const CeedScalar TwoMu = E/(1+nu);
  const CeedScalar Kbulk = E/(3*(1-2*nu)); //bulk Modulus
  const CeedScalar lambda = (3*Kbulk-TwoMu)/3;

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
    // Apply dXdx^-1 to deltadu = graddeltau
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
    //strain (epsilon)
    //    and
    //stress (sigma) in Voigt notation:
    //           [e11]              [sigma11]
    //           [e22]              [sigma22]
    // epsilon = [e33]  ,   sigma = [sigma33]
    //           [e23]              [sigma23]
    //           [e13]              [sigma13]
    //           [e12]              [sigma12]
    //
    // Sigma = S * epsilon
    //                         [1-nu   nu    nu                                    ]
    //                         [ nu   1-nu   nu                                    ]
    //                         [ nu    nu   1-nu                                   ]
    // S = E/((1+nu)*(1-2*nu)) [                  (1-2*nu)/2                       ]
    //                         [                             (1-2*nu)/2            ]
    //                         [                                        (1-2*nu)/2 ]

    //Above Voigt Notation is placed in a 3x3 matrix:
    const CeedScalar strain_vol = gradu[0][0][i] + gradu[1][1][i] + gradu[2][2][i];
    const CeedScalar lambda_bar = lambda/(1+strain_vol);

    const CeedScalar dsigma00 = (TwoMu+lambda_bar)*de[0][0] + lambda_bar*de[1][1] + lambda_bar*de[2][2],
                     dsigma11 = lambda_bar*de[0][0] + (TwoMu+lambda_bar)*de[1][1] + lambda_bar*de[2][2],
                     dsigma22 = lambda_bar*de[0][0] + lambda_bar*de[1][1] + (TwoMu+lambda_bar)*de[2][2],
                     dsigma12 = 0.5*TwoMu*de[1][2],
                     dsigma02 = 0.5*TwoMu*de[0][2],
                     dsigma01 = 0.5*TwoMu*de[0][1];
    const CeedScalar dsigma[3][3] = {
        {dsigma00, dsigma01, dsigma02},
        {dsigma01, dsigma11, dsigma12},
        {dsigma02, dsigma12, dsigma22}
      };

    // Apply dXdx^-T and weight
    for (int j=0; j<3; j++)     // Component
      for (int k=0; k<3; k++) { // Derivative
        deltadvdX[k][j][i] = 0;
        for (int m=0; m<3; m++)
          deltadvdX[k][j][i] += dXdx[k][m] * dsigma[j][m] * wJ;
      }
  } // End of Quadrature Point Loop

  return 0;
}

#endif //End of __HYPER_SS__H
