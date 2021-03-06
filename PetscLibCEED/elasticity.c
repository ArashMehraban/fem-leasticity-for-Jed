const char help[] = "Solve solid Problems with CEED and PETSc DMPlex\n";

#include <stdbool.h>
#include <string.h>
#include <petscksp.h>
#include <petscdmplex.h>
#include <ceed.h>
#include "setup.h"

static int dummyFun() { PetscFunctionBeginUser;  PetscFunctionReturn(0);}

int main(int argc, char **argv) {
  PetscInt    ierr;
  MPI_Comm    comm;
  AppCtx
  appCtx; //contains polinomial basis degree, problem choice & mesh filename
  Physics     phys;   //contains nu and E
  DM          dm;
  PetscInt    ncompu = 3;                 // 3 dofs in 3D
  Vec         U, Uloc, R, Rloc, F, Floc;  // loc: Local R:Residual
  PetscInt    Ugsz, Ulsz, Ulocsz;         // g:global, sz: size
  UserMult    resCtx, jacobCtx;;
  Mat         mat;
  //Ceed constituents
  char        ceedresource[PETSC_MAX_PATH_LEN] = "/cpu/self/ref/serial";
  Ceed        ceed;
  CeedData    ceeddata;
  SNES        snes;



  ierr = PetscInitialize(&argc, &argv, NULL, help);
  if(ierr)
    return ierr;

  comm = PETSC_COMM_WORLD;
  //set mesh-file, polynomial degree, problem type
  ierr = processCommandLineOptions(comm, &appCtx); CHKERRQ(ierr);
  //set Poison's ratio, Young's Modulus
  ierr = PetscMalloc1(1, &phys); CHKERRQ(ierr);
  ierr = processPhysics(comm, phys); CHKERRQ(ierr);
  //create distributed DM from mesh file (interpolate if polynomial degree > 1)
  ierr = createDistributedDM(comm, &appCtx, &dm); CHKERRQ(ierr);
  //setup DM by polinomial degree
  ierr = SetupDMByDegree(dm, &appCtx, ncompu);

  // Create Unknonw vector U and Residual Vector R
  ierr = DMCreateGlobalVector(dm, &U); CHKERRQ(ierr);
  ierr = VecGetSize(U, &Ugsz); CHKERRQ(ierr);
  ierr = VecGetLocalSize(U, &Ulsz); CHKERRQ(ierr); //For matShell

  ierr = DMCreateLocalVector(dm, &Uloc); CHKERRQ(ierr);
  ierr = VecGetSize(Uloc, &Ulocsz); CHKERRQ(ierr); //For libCeed

  ierr = VecDuplicate(U, &R); CHKERRQ(ierr);
  ierr = VecDuplicate(U, &F); CHKERRQ(ierr);
  ierr = VecZeroEntries(Uloc); CHKERRQ(ierr);
  ierr = VecDuplicate(Uloc, &Rloc); CHKERRQ(ierr);

  // Set up libCEED
  CeedInit(ceedresource, &ceed);
  ierr = PetscCalloc1(1, &ceeddata); CHKERRQ(ierr);
  // Create local forcing vector
  CeedVector forceceed;
  CeedScalar *f;
  if (appCtx.forcingChoice != FORCE_NONE) {
    ierr = VecDuplicate(Uloc, &Floc); CHKERRQ(ierr);
    CeedVectorCreate(ceed, Ulocsz, &forceceed);
    ierr = VecGetArray(Floc, &f); CHKERRQ(ierr);
    CeedVectorSetArray(forceceed, CEED_MEM_HOST, CEED_USE_POINTER, f);
  }
  // libCEED objects setup
  ierr = SetupLibceedByDegree(dm, ceed, &appCtx, phys, ceeddata, ncompu, Ugsz,
                              Ulocsz, forceceed); CHKERRQ(ierr);

  // Setup global forcing vector
  ierr = VecZeroEntries(F); CHKERRQ(ierr);
  if (appCtx.forcingChoice != FORCE_NONE) {
    ierr = VecRestoreArray(Floc, &f); CHKERRQ(ierr);
    ierr = DMLocalToGlobalBegin(dm, Floc, ADD_VALUES, F); CHKERRQ(ierr);
    ierr = DMLocalToGlobalEnd(dm, Floc, ADD_VALUES, F); CHKERRQ(ierr);
    CeedVectorDestroy(&forceceed);
    ierr = VecDestroy(&Floc); CHKERRQ(ierr);
  }

  // Setup SNES
  ierr = SNESCreate(comm, &snes); CHKERRQ(ierr);
  ierr = PetscMalloc1(1, &resCtx); CHKERRQ(ierr);
  // Jacobian context
  ierr = PetscMalloc1(1, &jacobCtx); CHKERRQ(ierr);
  ierr =  CreateMatrixFreeCtx(comm, dm, Uloc, ceeddata, ceed, resCtx, jacobCtx);
  CHKERRQ(ierr);
  resCtx->force = F;
  // Function that computes the residual
  ierr = SNESSetFunction(snes, R, FormResidual_Ceed, resCtx); CHKERRQ(ierr);
  // Form Action of Jacobian on delta_u
  ierr = MatCreateShell(comm, Ulsz, Ulsz, Ugsz, Ugsz, jacobCtx, &mat);
  CHKERRQ(ierr);
  ierr = MatShellSetOperation(mat, MATOP_MULT,
                              (void (*)(void))ApplyJacobian_Ceed); CHKERRQ(ierr);
  ierr = SNESSetJacobian(snes, mat, mat, dummyFun, NULL); CHKERRQ(ierr);
  // Set KSP options
  {
    PC pc;
    KSP ksp;
    ierr = SNESGetKSP(snes,&ksp); CHKERRQ(ierr);
    ierr = KSPSetType(ksp, KSPCG); CHKERRQ(ierr);
    ierr = KSPGetPC(ksp, &pc); CHKERRQ(ierr);
    ierr = PCSetType(pc, PCNONE); CHKERRQ(ierr); //For Now No Preconditioner
    ierr = KSPSetFromOptions(ksp);
  }
  ierr = SNESSetDM(snes, dm); CHKERRQ(ierr);
  ierr = SNESSetFromOptions(snes); CHKERRQ(ierr);

  // Initial Guess
  ierr = VecSet(U, 1.0); CHKERRQ(ierr);

  // Solve
//  ierr = SNESSolve(snes, NULL, U); CHKERRQ(ierr);

// Testing Only - Use KSP in place of SNES
/*
  PC pc;
  KSP ksp;
  ierr = SNESGetKSP(snes,&ksp); CHKERRQ(ierr);
  ierr = KSPSetType(ksp, KSPCG); CHKERRQ(ierr);
  ierr = KSPGetPC(ksp, &pc); CHKERRQ(ierr);
  ierr = PCSetType(pc, PCNONE); CHKERRQ(ierr); //For Now No Preconditioner
  ierr = KSPSetFromOptions(ksp);
  ierr = KSPSetOperators(ksp, mat, mat); CHKERRQ(ierr);
  ierr = KSPSolve(ksp, F, U); CHKERRQ(ierr);
*/

  // Compute error
  if (appCtx.forcingChoice == FORCE_MMS) {
    CeedScalar l2error, l2Unorm;
    const CeedScalar *truearray;
    Vec errorVec, trueVec;
    ierr = VecDuplicate(U, &errorVec); CHKERRQ(ierr);
    ierr = VecDuplicate(U, &trueVec); CHKERRQ(ierr);

    // Global true soltion vector
    CeedVectorGetArrayRead(ceeddata->truesoln, CEED_MEM_HOST, &truearray);
    ierr = VecPlaceArray(resCtx->Yloc, truearray); CHKERRQ(ierr);
    ierr = DMLocalToGlobalBegin(resCtx->dm, resCtx->Yloc, INSERT_VALUES, trueVec);
    CHKERRQ(ierr);
    ierr = DMLocalToGlobalEnd(resCtx->dm, resCtx->Yloc, INSERT_VALUES, trueVec);
    CHKERRQ(ierr);
    ierr = VecResetArray(resCtx->Yloc); CHKERRQ(ierr);
    CeedVectorRestoreArrayRead(ceeddata->truesoln, &truearray);

    // Compute error
    ierr = VecWAXPY(errorVec, -1.0, U, trueVec); CHKERRQ(ierr);
    ierr = VecNorm(errorVec, NORM_2, &l2error); CHKERRQ(ierr);
    ierr = VecNorm(U, NORM_2, &l2Unorm); CHKERRQ(ierr);
    l2error /= l2Unorm;

    ierr = PetscPrintf(comm, "L2 Error: %f\n", l2error); CHKERRQ(ierr);

// Testing Only, Check residual of true solution

    ierr = VecCopy(trueVec, U);
    ierr = SNESSolve(snes, F, U); CHKERRQ(ierr);

    // Compute error
    ierr = VecWAXPY(errorVec, -1.0, U, trueVec); CHKERRQ(ierr);
    ierr = VecNorm(errorVec, NORM_2, &l2error); CHKERRQ(ierr);
    ierr = VecNorm(U, NORM_2, &l2Unorm); CHKERRQ(ierr);
    l2error /= l2Unorm;

    ierr = PetscPrintf(comm, "L2 Error: %f\n", l2error); CHKERRQ(ierr);

    // Cleanup
    ierr = VecDestroy(&errorVec); CHKERRQ(ierr);
    ierr = VecDestroy(&trueVec); CHKERRQ(ierr);
  }

  // Free objects
  ierr = VecDestroy(&U); CHKERRQ(ierr);
  ierr = VecDestroy(&Uloc); CHKERRQ(ierr);
  ierr = VecDestroy(&R); CHKERRQ(ierr);
  ierr = VecDestroy(&Rloc); CHKERRQ(ierr);
  ierr = VecDestroy(&F); CHKERRQ(ierr);
  ierr = VecDestroy(&resCtx->Yloc); CHKERRQ(ierr);
  ierr = MatDestroy(&mat); CHKERRQ(ierr);
  ierr = DMDestroy(&dm); CHKERRQ(ierr);
  ierr = PetscFree(resCtx); CHKERRQ(ierr);
  ierr = PetscFree(jacobCtx); CHKERRQ(ierr);

  ierr = PetscFree(phys); CHKERRQ(ierr);
  ierr = CeedDataDestroy(0, ceeddata); CHKERRQ(ierr);
  CeedDestroy(&ceed);
  SNESDestroy(&snes);


  return PetscFinalize();
}
