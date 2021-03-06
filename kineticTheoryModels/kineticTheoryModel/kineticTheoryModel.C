/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2010 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "kineticTheoryModel.H"
#include "surfaceInterpolate.H"
#include "mathematicalConstants.H"
#include "fvCFD.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::kineticTheoryModel::kineticTheoryModel
(
    const Foam::phaseModel& phasea,
    const Foam::volVectorField& Ub,
    const Foam::volScalarField& alpha,
    const Foam::dragModel& draga
)
:
    phasea_(phasea),
    Ua_(phasea.U()),
    Ub_(Ub),
    alpha_(alpha),
    phia_(phasea.phi()),
    draga_(draga),

    rhoa_(phasea.rho()),
    da_(phasea.d()),
    nua_(phasea.nu()),

    kineticTheoryProperties_
    (
        IOobject
        (
            "kineticTheoryProperties",
            Ua_.time().constant(),
            Ua_.mesh(),
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        )
    ),
    kineticTheory_(kineticTheoryProperties_.lookup("kineticTheory")),
    equilibrium_(kineticTheoryProperties_.lookup("equilibrium")),

    viscosityModel_
    (
        kineticTheoryModels::viscosityModel::New
        (
            kineticTheoryProperties_
        )
    ),
    conductivityModel_
    (
        conductivityModel::New
        (
            kineticTheoryProperties_
        )
    ),
    radialModel_
    (
        radialModel::New
        (
            kineticTheoryProperties_
        )
    ),
    granularPressureModel_
    (
        granularPressureModel::New
        (
            kineticTheoryProperties_
        )
    ),
    frictionalStressModel_
    (
        frictionalStressModel::New
        (
            kineticTheoryProperties_
        )
    ),
    e_(kineticTheoryProperties_.lookup("e")),
    alphaMax_(kineticTheoryProperties_.lookup("alphaMax")),
    alphaMinFriction_(kineticTheoryProperties_.lookup("alphaMinFriction")),
    DiluteCut_(kineticTheoryProperties_.lookup("DiluteCut")),
    ttzero(kineticTheoryProperties_.lookup("ttzero")),
    ttone(kineticTheoryProperties_.lookup("ttone")),
    MaxTheta(kineticTheoryProperties_.lookup("MaxTheta")),
    Jcoff1_(kineticTheoryProperties_.lookup("KineticJ1")),
    Jcoff2_(kineticTheoryProperties_.lookup("KineticJ2")),
    Jcoff3_(kineticTheoryProperties_.lookup("KineticJ3")),
    Fr_(kineticTheoryProperties_.lookup("Fr")),
    eta_(kineticTheoryProperties_.lookup("eta")),
    p_(kineticTheoryProperties_.lookup("p")),
    phi_(dimensionedScalar(kineticTheoryProperties_.lookup("phi"))*M_PI/180.0),
    Theta_
    (
        IOobject
        (
            "Theta",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        Ua_.mesh()
    ),
    mua_
    (
        IOobject
        (
            "mua",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -1, 0, 0), 0.0)
    ),
    lambda_
    (
        IOobject
        (
            "lambda",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -1, 0, 0), 0.0)
    ),
    pa_
    (
        IOobject
        (
            "pa",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -2, 0, 0), 0.0)
    ),
    pf
    (
        IOobject
        (
            "pf",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -2, 0, 0), 0.0)
    ),
    ppMagf_
    (
        IOobject
        (
            "ppMagf",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -2, 0, 0), 0.0)
    ),
    kappa_
    (
        IOobject
        (
            "kappa",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(1, -1, -1, 0, 0), 0.0)
    ),
    gs0_
    (
        IOobject
        (
            "gs0",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(0, 0, 0, 0, 0), 1.0)
    ),
    gs0Prime_
    (
        IOobject
        (
            "gs0prime",
            Ua_.time().timeName(),
            Ua_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        Ua_.mesh(),
        dimensionedScalar("zero", dimensionSet(0, 0, 0, 0, 0), 0.0)
    )
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::kineticTheoryModel::~kineticTheoryModel()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::kineticTheoryModel::solve(const volTensorField& gradUat, const volScalarField& kb,const volScalarField& epsilonb,const volScalarField& nutf,const dimensionedScalar& B, const dimensionedScalar& tt)
{
    if (!kineticTheory_)
    {
        return;
    }

    const scalar sqrtPi = sqrt(constant::mathematical::pi);

//    surfaceScalarField phi = 1.5*rhoa_*phia_*fvc::interpolate(max(alpha_,DiluteCut_));
    surfaceScalarField phi = 1.5*rhoa_*phia_*fvc::interpolate((alpha_+1e-6));
    volTensorField dUU = gradUat.T();         //that is fvc::grad(Ua_);
    volSymmTensorField D = symm(dUU);         //0.5*(dU + dU.T)
    volTensorField dU = dUU;//dev(dUU);

    // NB, drag = K*alpha*beta,
    // (the alpha and beta has been extracted from the drag function for
    // numerical reasons)
    // this is inconsistent with momentum equation, since the following form missed
    // the drift velocity
    volScalarField Ur_ = mag(Ua_ - Ub_);
//    volScalarField Ur = mag(Ua_ - Ub_ + nuft/(max(alpha_,1e-9)*(1.0-alpha_))*fvc::grad(alpha_));
//    volScalarField Ur = mag(Ua_ - Ub_ + nuft/(max(alpha_,1e-9))*fvc::grad(alpha_));    
    volScalarField betaPrim = (1.0 - alpha_)*draga_.K(Ur_);

    // Calculating the radial distribution function (solid volume fraction is
    //  limited close to the packing limit, but this needs improvements)
    //  The solution is higly unstable close to the packing limit.
    gs0_ = radialModel_->g0
    (
        min(alpha_, alphaMax_ - 1e-9),
        alphaMax_
    );

    // particle pressure - coefficient in front of Theta (Eq. 3.22, p. 45)
    volScalarField PsCoeff = granularPressureModel_->granularPressureCoeff
    (
        alpha_,
//        (alpha_+1e-12),
        gs0_,
        rhoa_,
        e_
    );

    // 'thermal' conductivity (Table 3.3, p. 49)
    kappa_ = conductivityModel_->kappa(alpha_, Theta_, gs0_, rhoa_, da_, e_);
    // particle viscosity (Table 3.2, p.47)
    mua_ = viscosityModel_->mua(alpha_, min(Theta_,max(0.667*max(kb),0.0*kb)), gs0_, rhoa_, da_, e_);

    dimensionedScalar Tsmall
    (
        "small",
        dimensionSet(0 , 2 ,-2 ,0 , 0, 0, 0),
        scalar(1.0e-40)
    );

    dimensionedScalar TsmallEqn
    (
        "small",
        dimensionSet(0 , 2 ,-2 ,0 , 0, 0, 0),
        scalar(1.0e-30)
    );

    dimensionedScalar TsmallSqrt = sqrt(Tsmall);

    volScalarField ThetaSqrt = sqrt(Theta_);

    volScalarField muaa = rhoa_*da_*
    (
        (4.0/5.0)*sqr(alpha_)*gs0_*(1.0 + e_)/sqrtPi
      + (1.0/15.0)*sqrtPi*gs0_*(1.0 + e_)*sqr(alpha_)
      + (1.0/6.0)*sqrtPi*alpha_
      + (10.0/96.0)*sqrtPi/((1.0 + e_)*gs0_)
    )/(ThetaSqrt+TsmallSqrt);

    // dissipation (Eq. 3.24, p.50)
    volScalarField gammaCoeff =
              3.0*(1.0 - sqr(e_))*sqr(alpha_)*rhoa_*gs0_*((4.0/da_)*ThetaSqrt/sqrtPi-tr(D));
//              3.0*(1.0 - sqr(e_))*sqr(alpha_)*rhoa_*gs0_*((4.0/da_)*ThetaSqrt/sqrtPi);
    // Eq. 3.25, p. 50 Js = J1 - J2

// The following is to calculate parameter tmf_ in u_f*u_s correlation
    dimensionedScalar Tpsmall_
    (
         "Tpsmall_",
         dimensionSet(1, -1, -3, 0, 0, 0, 0),
         scalar(1e-30)
    );
    volScalarField tmf_ = Foam::exp(-B*rhoa_*scalar(6.0)*epsilonb/(max(kb*betaPrim,Tpsmall_)));

    volScalarField J1 = 3.0*alpha_*betaPrim;
    volScalarField J2 =
        0.25*alpha_*sqr(betaPrim)*da_*sqr(Ur_)
       /(rhoa_*sqrtPi*((ThetaSqrt+TsmallSqrt)));

    // bulk viscosity  p. 45 (Lun et al. 1984).
    lambda_ = (4.0/3.0)*sqr(alpha_)*rhoa_*da_*gs0_*(1.0+e_)*sqrt(min(Theta_,max(0.667*max(kb),0.0*kb)))/sqrtPi;
//    volScalarField lambdaa = (4.0/3.0)*sqr(alpha_)*rhoa_*da_*gs0_*(1.0+e_)/(sqrtPi*(ThetaSqrt+TsmallSqrt));

    // stress tensor, Definitions, Table 3.1, p. 43
    volSymmTensorField tau = 2.0*mua_*D + (lambda_ - (2.0/3.0)*mua_)*tr(D)*I;
//    volSymmTensorField tau = scalar(2.0)*muaa*D + (lambdaa - (scalar(2.0)/scalar(3.0))*muaa)*tr(D)*I;

    dimensionedScalar alphasmall
    (
           "alphasmall",
           dimensionSet(0, 0, 0, 0, 0, 0, 0),
           scalar(1e-9)
    );

    if (!equilibrium_)
    {
        // construct the granular temperature equation (Eq. 3.20, p. 44)
        // NB. note that there are two typos in Eq. 3.20
        // no grad infront of Ps
        // wrong sign infront of laplacian
        fvScalarMatrix ThetaEqn
        (
           fvm::ddt(1.5*(alpha_+1e-6)*rhoa_, Theta_)
         + fvm::div(phi, Theta_, "div(phi,Theta)")
         ==
            //Ps term.
          - fvm::SuSp(((PsCoeff*I) && dU), Theta_)
            //production due to shear.
//          + fvm::SuSp((tau && dU),Theta_)
          + (tau && dU)
            // granular temperature conduction.
          + fvm::laplacian(kappa_, Theta_, "laplacian(kappa,Theta)")
            //energy disipation due to inelastic collision.
          - fvm::SuSp(gammaCoeff, Theta_)
            //
          - fvm::SuSp(J1, Theta_)
          + scalar(0.6666667)*J1*tmf_*kb
        );

        ThetaEqn.relax();
        ThetaEqn.solve();
    }
    else
    {
        // equilibrium => dissipation == production
        // Eq. 4.14, p.82
        volScalarField K1 = 2.0*(1.0 + e_)*rhoa_*gs0_;
        volScalarField K3 = 0.5*da_*rhoa_*
            (
                (sqrtPi/(3.0*(3.0-e_)))
               *(1.0 + 0.4*(1.0 + e_)*(3.0*e_ - 1.0)*alpha_*gs0_)
               +1.6*alpha_*gs0_*(1.0 + e_)/sqrtPi
            );

        volScalarField K2 =
            4.0*da_*rhoa_*(1.0 + e_)*alpha_*gs0_/(3.0*sqrtPi) - 2.0*K3/3.0;

        volScalarField K4 = 12.0*(1.0 - sqr(e_))*rhoa_*gs0_/(da_*sqrtPi);

        volScalarField trD = tr(D);
        volScalarField tr2D = sqr(trD);
        volScalarField trD2 = tr(D & D);

//        volScalarField t1 = K1*alpha_ + rhoa_;
        volScalarField t1 = K1*alpha_;
        volScalarField l1 = -t1*trD;
        volScalarField l2 = sqr(t1)*tr2D;
        volScalarField l3 = 4.0*K4*alpha_*(2.0*K3*trD2 + K2*tr2D);

        Theta_ = sqr((l1 + sqrt(l2 + l3))/(2.0*(alpha_ + 1.0e-4)*K4));
    }
// Following is for the initial stability.
    forAll(alpha_, cellk)
    {      
       if(tt.value()>=ttone.value() && (alpha_[cellk] < DiluteCut_.value()))
       {
          Theta_[cellk] =min(Theta_[cellk],0.667*kb[cellk]);
       }  
    }   
    Theta_.correctBoundaryConditions();

    Theta_.max(1.0e-20);
    Theta_.min(MaxTheta);

// need to update after solving Theta Equation.
    PsCoeff = granularPressureModel_->granularPressureCoeff
    (
        alpha_,
        gs0_,
        rhoa_,
        e_
    );
// update bulk viscosity and shear viscosity
// bulk viscosity  p. 45 (Lun et al. 1984).
    lambda_ = (4.0/3.0)*sqr(alpha_)*rhoa_*da_*gs0_*(1.0+e_)*sqrt(Theta_)/sqrtPi;
// particle viscosity (Table 3.2, p.47)
    mua_ = viscosityModel_->mua(alpha_, Theta_, gs0_, rhoa_, da_, e_);

    pf = frictionalStressModel_->frictionalPressure
    (
        alpha_,
        alphaMinFriction_,
        alphaMax_,
        Fr_,
        eta_,
        p_
    );
//  yes, after solving Theta, because frictional stress is not a part of kinetic theoty
//    PsCoeff = PsCoeff + pf/(max(Theta_,Tsmall));

    PsCoeff.min(1e+6);
    PsCoeff.max(-1e+6);

    // update particle pressure, just from the kinetic part
    pa_ = PsCoeff*Theta_;

    // frictional shear stress, Eq. 3.30, p. 52
    volScalarField muf = frictionalStressModel_->muf
    (
        alpha_,
        alphaMinFriction_,
        alphaMax_,
        pf,
        D,
        phi_
    );

   // add frictional stress
    mua_ = mua_+muf; /////////////////////////////////////// please check this out
    mua_.max(0.0);
    Info<< "kinTheory: max(Theta) = " << max(Theta_).value() <<" min(Theta) = "<<min(Theta_).value()<< endl;

    volScalarField ktn = mua_/rhoa_;

    Info<< "kinTheory: min(nua) = " << min(ktn).value()
        << ", max(nua) = " << max(ktn).value() << endl;

    Info<< "kinTheory: min(pa) = " << min(pa_).value()
        << ", max(pa) = " << max(pa_).value() << endl;
}

volScalarField& Foam::kineticTheoryModel::ppMagf(const volScalarField& alphaUpdate)
{
    volScalarField alpha = alphaUpdate;

      gs0_ = radialModel_->g0(min(alpha, alphaMax_ - 1e-9), alphaMax_); 
      gs0Prime_ = radialModel_->g0prime(min(alpha, alphaMax_ - 1e-9), alphaMax_);
    
    // Computing ppMagf
    // The original code made a mistake here, Pa = Coff*Theta, then the d_Pa/d_alpha should be
    // (d_Coff/d_alpha) *Theta + Coff* (d_Theta/d_alpha)
    ppMagf_ = Theta_*granularPressureModel_->granularPressureCoeffPrime
    (
	alpha, 
	gs0_, 
	gs0Prime_, 
	rhoa_, 
	e_
    );

    volScalarField ppMagfFriction = frictionalStressModel_->frictionalPressurePrime
    (
	alpha, 
	alphaMinFriction_, 
	alphaMax_,
        Fr_,
        eta_,
        p_
    );

    // NOTE: this might not be appropriate if J&J model is used (verify)
//    ppMagf_ = max(ppMagf_,ppMagfFriction);
//      ppMagf_ = ppMagf_ + ppMagfFriction;
      forAll(alpha, cellI)
    {
	if(alpha[cellI] >= alphaMinFriction_.value())
	{
	    ppMagf_[cellI] = ppMagfFriction[cellI];
	}
    }

    ppMagf_.min(1.0e+5);
    ppMagf_.max(-1.0e+5);    
    ppMagf_.correctBoundaryConditions();
    
    return ppMagf_;
    
}


// ************************************************************************* //
