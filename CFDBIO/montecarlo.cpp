#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <math.h>
#include <time.h>

#include "montecarlo.h"
#include "Parameter.h"

using namespace std;

MonteCarlo::MonteCarlo(){
}

MonteCarlo::~MonteCarlo(){
}

void MonteCarlo::MC_ParticleTracingParameter3D(){

    ModeT=1;

    //if using 3D field, lx ly lz is smae with meshing
    //if using 2D field, have to match the  coordinate system manually.
    lx=400*1000;
    ly=50*1000;
    lz=200*1000;

    /*ModeT=1, BM
     *ModeT=2, EO
     *ModeT=3, BMEO
     *ModeT=4, EP
     *ModeT=5, BMEP
     *ModeT=6, BMEOEP
     */

    //Sensor Area
    SensorWidth=20*1000;
    SensorLength=50*1000;
    ReceptorMesh=10;

    //
    WireWidth=20;
    WireSpacing=80;  //Wire edge to wire edge

    WireN=SensorWidth/(WireWidth+WireSpacing);

    t=1000;        //time, s.
    tau=1e-3;   //time step
    TN=t/tau+1; //nubmer of steps

    AnalyteRadius_m=5e-9;     //Particle radius [m]

    AnalyteRadius_nm=AnalyteRadius_m*1e9;
    AnalyteMass=150*1000*1.6605e-27; //1.6605e-27*1000*150; //mass, kg. 1 Dalton = 1.6605e-27 kg
    AnalyteValence=-10;
    DotN=964;   //1mM = 1 mol/m3, Avogadro = 6.0221409e23

    int Output_frame=200;

    OutputTimeStepsPerFrame=0.05/tau; //Each ouput trajactory frame contains X tau steps.
    OutputTime=Output_frame*0.05;     //Print trajactory within X sec
    OutputMoleculeNumber=DotN;  //output first X molecules trajtory
    ReportTimeSection=10; // Report on screen each X sec.

    D=kb*Tamb/(6*M_PI*eta_m*AnalyteRadius_m);
    gamma=6*M_PI*eta_m*AnalyteRadius_m;

    tauB=AnalyteMass/gamma;

    cout <<"t="<<t<< " tau="<<tau<<endl;
    //cout <<"tauB="<<tauB<<endl;

    SigmaDist=sqrt(2*D*tau); //Standard Deviation for random displacement, m.

    g=2*gamma*kb*Tamb;
    SigmaForce=sqrt(g/tau); //Standard Deviation for random force, N.

    //cout << SigmaForce/AnalyteMass*tauB*tau*1e9 << endl;
    //cout << SigmaDist*1e9  << endl;

    sigma=1;
    mean=0;


    cout <<"D="<< D*1e18 <<" [nm^2/s], sqrt(D*tau)="<<sqrt(D*1e18*tau)<<" [nm]"<<endl;

    Wb=SigmaDist*1e9; // Brownian Motion depletion width, nm.

    //cout << "Wb="<<Wb<<endl;

    Rpx=Rpy=1;
    //receptor Parameter
    //The real useful Rpx Rpy are between the meshing points.
    //So the edge point (Rpx-1 Rpy-1) is not used, they are build for bubble search
    Rpx=Rpx+SensorWidth/ReceptorMesh;
    Rpy=Rpy+SensorLength/ReceptorMesh;
    RL=(Rpx)*(Rpy);

    UnitCon=1/Avogadro/(ReceptorMesh*ReceptorMesh*1e-18);
    //cout << UnitCon << " Mole/m2" << endl;

    C_A=(DotN/6.02e23)/((lx*ly*lz)*1e-27);
    C_B=10*UnitCon;
    C_AB=0;

    k_forward=0;
    k_backward=0;

    k_forward1=1e3;
    k_backward1=1e-3;

    k_forward2=0;
    k_backward2=1e4;

    //Pre-Calculation

    //Local A concentration
    LC_A=1/Avogadro/(ReceptorMesh*ReceptorMesh*ReceptorMesh*1e-27)*1e3; // 1e3*Mole/m3 = 1e3*mM = M
    LC_A_cylindrical=1/Avogadro/(M_PI*AnalyteRadius_nm*AnalyteRadius_nm*2*AnalyteRadius_nm*1e-27)*1e3; // 1e3*Mole/m3 = 1e3*mM = M


    //Reaction
    //R1=k1[A][B]
    //     MA/s = 1/(M*s) * M  * MA
    double R1_cylindrical  =k_forward1*LC_A_cylindrical*C_B;

    //     MA   = MA/s * s
    double R1MA_cylindrical = R1_cylindrical   * tau;

    R1N_cylindrical = R1MA_cylindrical*Avogadro*(ReceptorMesh*1e-9)*(ReceptorMesh*1e-9);

    ZetaV=0;

    //R1=k1[A][B]
    //     MA/s = 1/(M*s) * M  * MA
    double R1  =k_forward1*LC_A*C_B;
    //     MA   = MA/s * s
    double R1MA = R1   * tau;
    double R1N = R1MA*Avogadro*(ReceptorMesh*1e-9)*(ReceptorMesh*1e-9);

    MaximumCaptureNumberRSA2=SensorLength*SensorWidth*0.547/(M_PI*AnalyteRadius_nm*AnalyteRadius_nm);


    double Vrms=sqrt(3*2*D);
    double Volume=lx*ly*lz*1e-27;
    double N_collision=0.25*DotN*Vrms/(Volume);
    double N_collisiononSensor=N_collision*SensorLength*SensorWidth*1e-18;

    /*
    cout <<"Maximum capture probability per time step = R1N="<<R1N << endl;
    cout <<"Maximum capture probability per time step for RSA2= R1N_cylindrical="<<R1N_cylindrical << endl;
    cout <<"Maximun tau for first order boundary = " << tau/R1N << endl;
    cout <<"Maximun tau for first order boundary for RSA2 = " << tau/R1N_cylindrical << endl;
    cout <<"Release probability per time step = k_backward1*tau ="<<k_backward1*tau<<endl;
    cout <<"Release probability per time step = k_backward2*tau ="<<k_backward2*tau<<endl;
    cout <<"Maximum capture number on sensor for RSA2= MaximumCaptureNumber ="<<MaximumCaptureNumberRSA2 << endl;
    cout <<"Number of collisions per unit time per area ="<<N_collision<<" N/(m2*s)" << endl;
    cout <<"Number of collisions per unit time on sensor ="<<N_collisiononSensor<<" N/s" << endl;
    cout <<"N_AB x k_backward1 = N_collisiononSensor, AB = "<<N_collisiononSensor/k_backward1<<" N/s" << endl;
    */
    if(lz!=0){
        cout << "DotN="<<DotN<<" Mole Concentration="<<(DotN/6.02e23)/((lx*ly*lz)*1e-27)<<" mol/m3 ="<< C_A*1e3<<" M"<<endl;
    }else{
        cout << "DotN="<<DotN<<" Mole Concentration="<<(DotN/6.02e23)/((lx*ly)*1e-18)<<" mol/m2"<<endl;
    }

    fstream output;
    output.open("ParticleTracingParameter3D.txt", fstream::out | fstream::trunc);

    switch(ModeT){

    case 1:
        output << "1.Brownian" << endl;
        break;

    case 2:
        output << "2.Electroosmosis" << endl;
        break;

    case 3:
        output << "3.Brownian + Electroosmosis" << endl;
        break;

    case 4:
        output << "4.Electrophoresis" << endl;
        break;

    case 5:
        output << "5.Brownian + Electrophoresis" << endl;
        break;

    case 6:
        output << "6.Brownian + Electroosmosis + Electrophoresis" << endl;
        break;

    default:
        output << "Undifined Simulation ModeT." << endl;
        cout << "Undifined Simulation ModeT." << endl;
        exit(0);
    }

    output << "lx="<<lx<< " ly="<<ly<< " lz="<<ly<<endl;
    output << "t="<<t<<" tau="<<tau<< " TN="<<TN<<endl;
    output << "tauB="<<tauB<<endl;
    output << "D=" << D*1e18 <<" [nm^2/s], sqrt(D*tau)="<<sqrt(D*1e18*tau)<<" [nm]"<<endl;
    output << "AnalyteRadius_m="<<AnalyteRadius_m<<endl;
    output << "AnalyteMass="<<AnalyteMass<<endl;
    output << "AnalyteValence="<<AnalyteValence<<endl;
    output << "DotN="<<DotN<<endl;
    output << "Mole Concentration="<<C_A<<" mol/m3 ="<< C_A*1e3<<" M"<<endl;
    output << "SensorWidth="<<SensorWidth<<endl;
    output << "SensorLength="<<SensorLength<<endl;
    output << "ReceptorMesh="<<ReceptorMesh<<endl;
    output << "WireWidth="<<WireWidth<<endl;
    output << "WireSpacing="<<WireSpacing<<endl;
    output << "WireN="<<WireN<<endl;
    output << "Rpx="<<Rpx<< ", Rpy="<<Rpy<< ", RL="<<RL<<endl;
    if (tauB*100>tau){
        output << "tau is not small enough to ignore tauB! BM is not valid, Langevin is required." << endl;
        cout << "tau is not small enough to ignore tauB! BM is not valid, Langevin is required." << endl;
        //exit(0);
    }
    output << "C_B="<<C_B << " C_AB="<<C_AB<<endl;
    output << "k_forward1="<<k_forward1 << " k_backward1="<<k_backward1<<endl;
    output << "k_forward2="<<k_forward2 << " k_backward2="<<k_backward2<<endl;


    output <<"Maximum capture probability per time step = R1N="<<R1N << endl;
    output <<"Maximun tau for first order boundary = " << tau/R1N << endl;
    output <<"Release probability per time step = k_backward1*tau ="<<k_backward1*tau<<endl;
    output <<"Release probability per time step = k_backward2*tau ="<<k_backward2*tau<<endl;
    output <<"Maximun AB is (Rpx-1)*(Rpy-1)*10 = "<< (Rpx-1)*(Rpy-1)*10 << endl;
    output <<"Maximum capture number on sensor for RSA2 = MaximumCaptureNumber ="<<MaximumCaptureNumberRSA2 << endl;
    output <<"Number of collisions per unit time per area ="<<N_collision<<" N/(m2*s)" << endl;
    output <<"Number of collisions per unit time on sensor ="<<N_collisiononSensor<<" N/s" << endl;



    output.close();
}

void MonteCarlo::MC_ParticleTracingNew(){

    Analyte=new target*[DotN];

#pragma omp parallel for
    for(int i=0;i<DotN;i++){
        Analyte[i]=new target[2];
    }

    //ReceptorArray=new receptor [RL];

    ParticleOnWire=new int [WireN];
}

void MonteCarlo::MC_ParticleTracingInitialize3D(){

    srand(time(NULL));

#pragma omp parallel for
    for(int i=0;i<DotN;i++){
        for(int j=0;j<2;j++){
            Analyte[i][j].coordX=0;
            Analyte[i][j].coordY=0;
            Analyte[i][j].coordZ=0;
            Analyte[i][j].flag=-1;
        }
    }

#pragma omp parallel for
    for(int i=0;i<DotN;i++){
        Analyte[i][0].coordX=rand()/(double)RAND_MAX*(lx); // initial coordinate
        Analyte[i][0].coordY=rand()/(double)RAND_MAX*(ly);
        Analyte[i][0].coordZ=rand()/(double)RAND_MAX*(lz);
        //Analyte[i][0].coordX=lx/2-0.1*lx + rand()/(double)RAND_MAX*(0.2*lx); // initial coordinate
        //Analyte[i][0].coordY=ly/2-0.1*ly + rand()/(double)RAND_MAX*(0.2*ly);
        //Analyte[i][0].coordZ=rand()/(double)RAND_MAX*(0.1*lz);
    }

    /*
    double deltaX=ReceptorMesh;
    double deltaY=ReceptorMesh;

#pragma omp parallel for
    for(int j=0;j<Rpy;j++){
        for(int i=0;i<Rpx;i++){
            int pointer=Rpx*(j)+(i);
            ReceptorArray[pointer].coordX=(lx)/2-SensorWidth/2+deltaX/2+deltaX*i;
            ReceptorArray[pointer].coordY=(ly)/2-SensorLength/2+deltaY/2+deltaY*j;
            ReceptorArray[pointer].A=0;
            ReceptorArray[pointer].B=C_B;
            ReceptorArray[pointer].AB=C_AB;
        }
    }
    */

#pragma omp parallel for
    for(int i=0;i<WireN;i++){
        ParticleOnWire[i]=0;
    }

}

void MonteCarlo::MC_ParticleTracingSimulation3D(){


    int TotalCapture_now=0;
    int TotalCapture_previous=0;
    int CapturedOnSensor_now=0;
    int TotalParticleOnWires_now=0;
    int TotalParticleOnWires_previous=0;
    int TotalExcitedWire=0;

    stringstream Statistic1;
    string Statistic2;
    Statistic1<<"Statistic"<<ModeT<<".txt";
    Statistic2=Statistic1.str();

    // output 1 print Statistic
    fstream output1;
    output1.open(Statistic2.c_str(), fstream::out | fstream::trunc);
    output1.precision(7);
    output1 << "T(1)\tN(2)\tAll(3)\tIn(4)\tOut(5)\tReff(6)\tRSA sat. R(7) #"<<endl;
    output1 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    output1 << 0*tau << '\t'<< 0 << '\t'<< TotalCapture_now << '\t' << CapturedOnSensor_now <<'\t'<< TotalCapture_now-CapturedOnSensor_now  <<'\t' << CapturedOnSensor_now/(double)TotalCapture_now<<'\t'<< 0 <<endl;

    stringstream ParticleOnWire1;
    string ParticleOnWire2;
    ParticleOnWire1<<"ParticleOnWire"<<ModeT<<".txt";
    ParticleOnWire2=ParticleOnWire1.str();

    // output 4 print ParticleOnWire
    fstream output4;
    output4.open(ParticleOnWire2.c_str(), fstream::out | fstream::trunc);
    output4 << "T(1)\tExcitedW(2)\tTotalN(3)\tWire_1(4)\tWire_2(5)\tWire_3(6)\t #"<<endl;
    output4 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    output4 << 0*tau << '\t'<< 0 << '\t' << TotalParticleOnWires_now << '\t' ;
    for(int i=0;i<WireN;i++){
        output4 << ParticleOnWire[i] << '\t';
    }
    output4 << endl;


    // output 2 print Traj
    MC_PrintTrojactory(0);

    /*
     *ModeT=1, BM
     *ModeT=2, EO
     *ModeT=3, BMEO
     *ModeT=4, EP
     *ModeT=5, BMEP
     *ModeT=6, BMEOEP8
     */

    //k_forward=k_forward1;
    //k_backward=k_backward1;

    int StepUnit=ReportTimeSection/tau;

    int j=0;

    do{
        TotalParticleOnWires_previous=TotalParticleOnWires_now;
        TotalCapture_previous=TotalCapture_now;

        j=j+1;

        if(j%StepUnit==0){
            cout << "T=" << j*tau << " seconds. "<< j*tau/t*100 <<"% " << endl;
        }

        #pragma omp parallel for
        for(int i=0;i<DotN;i++){

            /*
            if(j==TN/2){
                k_forward=k_forward2;
                k_backward=k_backward2;
            }

            if((Analyte[i][1].flag!=-1) && (rand()/double(RAND_MAX)<=k_backward*tau)){
                //release
                //FirstOrderRelease(i,1);
                FirstOrderRelease_FiniteR(i,1);
            }
            */

            if(Analyte[i][1].flag==-1){

                // if particle is not captured.

                double sumX=0,sumY=0,sumZ=0;

                //BM force
                if(ModeT==1||ModeT==3||ModeT==5||ModeT==6){
                    sumX=sumX+SigmaDist*1e9*MC_NormalDistribution();
                    sumY=sumY+SigmaDist*1e9*MC_NormalDistribution();
                    sumZ=sumZ+SigmaDist*1e9*MC_NormalDistribution();
                }


                //Flow FieldX
                if(ModeT==2||ModeT==3||ModeT==6){

                    double avgVx=0;
                    double avgVy=0;
                    double avgVz=0;
                    //calculate pointer
                    int I=0,J=0,K=0;
                    int pointer = 0;

                    /*
                    //Using 3D Field
                    //Find IJK
                    I=MC_ParticleTracing3DFindI(i, 0);
                    J=MC_ParticleTracing3DFindJ(i, 0);
                    K=MC_ParticleTracing3DFindK(i, 0);
                    pointer = (px)*(py)*(K) + (px)*(J) + (I);
                    avgVx=CFDmaterial[pointer].ui;
                    avgVy=CFDmaterial[pointer].vi;
                    avgVz=CFDmaterial[pointer].wi;
                    */

                    /*
                    */
                    //Using 2D Field
                    //Find IJK
                    I=MC_ParticleTracing3DFindI(i, 0);
                    J=MC_ParticleTracing3DFindJ_In2DField(i, 0);
                    pointer = (px)*(J) + (I);

                    avgVx=CFDmaterial[pointer].ui;
                    avgVy=0;
                    avgVz=CFDmaterial[pointer].vi;

                    sumX=sumX+avgVx*tau;
                    sumY=sumY+avgVy*tau;
                    sumZ=sumZ+avgVz*tau;
                }

                TotalCapture_previous=TotalCapture_now;

                /*
                //EP forceX
                if(ModeT==4||ModeT==5||ModeT==6){
                    sumX=sumX+(80*(e0*1e9)*CFDmaterial[pointer].Ex*ZetaV/eta_m*tau)*1e9;
                    sumY=sumY+(80*(e0*1e9)*CFDmaterial[pointer].Ey*ZetaV/eta_m*tau)*1e9;
                    sumZ=sumZ+(80*(e0*1e9)*CFDmaterial[pointer].Ez*ZetaV/eta_m*tau)*1e9;
                }
                */


                //total XYZ
                Analyte[i][1].coordX=Analyte[i][0].coordX+sumX;
                Analyte[i][1].coordY=Analyte[i][0].coordY+sumY;
                Analyte[i][1].coordZ=Analyte[i][0].coordZ+sumZ;

                //BC
                //Stick
                //MC_StickBoundary_onSensor(i,1);
                //MC_StickBoundary_BottomSurface(i,1);
                //RSA
                MC_RSABoundary_onSensor(i,1);
                //MC_RSABoundary_BottomSurface(i,1);
                //FirstOrderBoundary
                //MC_FirstOrderBoundary(i,1);
                //MC_FirstOrderBoundary_FiniteR(i,1);
                //MC_FirstOrderBoundary_RSA1(i,1);
                //MC_FirstOrderBoundary_RSA2(i,1);

            }else{
                Analyte[i][1].coordX=Analyte[i][0].coordX;
                Analyte[i][1].coordY=Analyte[i][0].coordY;
                Analyte[i][1].coordZ=Analyte[i][0].coordZ;
            }
        }


        //Reset Counter
        TotalCapture_now=0;
        CapturedOnSensor_now=0;
        TotalParticleOnWires_now=0;
        TotalExcitedWire=0;

        for(int i=0;i<WireN;i++){
            ParticleOnWire[i]=0;
        }

        //Counting
        for(int i=0;i<DotN;i++){
            if(Analyte[i][1].flag!=-1){
                TotalCapture_now=TotalCapture_now+1;

                //Rectangular Sensor
                if((Analyte[i][1].coordY>=ly/2-SensorLength/2)&&(Analyte[i][1].coordY<=ly/2+SensorLength/2)){
                    if((Analyte[i][1].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][1].coordX<=lx/2+SensorWidth/2)){
                        //cout << Analyte[i][1].coordX <<" "<< Analyte[i][1].coordX<<" "<<lx/2-SensorWidth/2<<" "<<lx/2+SensorWidth/2<<" "<<ly/2-SensorLength/2<<" "<<ly/2+SensorLength/2<< endl;
                        CapturedOnSensor_now=CapturedOnSensor_now+1;
                    }
                }

                //On wire
                for(int j=0;j<WireN;j++){
                    double A=Analyte[i][1].coordX-(lx/2-SensorWidth/2);
                    double B=(((WireWidth+WireSpacing)/2+j*WireWidth+WireSpacing));
                    if(abs(A-B)< WireWidth/2){
                        TotalParticleOnWires_now=TotalParticleOnWires_now+1;
                        ParticleOnWire[j]=ParticleOnWire[j]+1;
                    }
                }
            }
        }


        //Excited Wire
        for(int j=0;j<WireN;j++){
            if(ParticleOnWire[j]!=0){
                TotalExcitedWire=TotalExcitedWire+1;
            }
        }

        // Output when total capture particles is increased.
        if(TotalCapture_now!=TotalCapture_previous){
            output1 << j*tau << '\t'<< j << '\t'<< TotalCapture_now << '\t' << CapturedOnSensor_now <<'\t'<< TotalCapture_now-CapturedOnSensor_now  <<'\t' << CapturedOnSensor_now/(double)TotalCapture_now<<'\t'<<((double)TotalCapture_now)/((double)MaximumCaptureNumberRSA2)<<endl;
        }

        // Output when total capture particles on wire is increased.
        if(TotalParticleOnWires_now!=TotalParticleOnWires_previous){
            output4 << j*tau << '\t'<< TotalExcitedWire << '\t' << TotalParticleOnWires_now << '\t' ;
            for(int i=0;i<WireN;i++){
                output4 << ParticleOnWire[i] << '\t';
            }
            output4 << endl;
        }


        /*
        //Output Traj
        if(j*tau<=OutputTime){
            if(j%OutputTimeStepsPerFrame==0){

                int tail=int(j/OutputTimeStepsPerFrame);
                MC_PrintTrojactory(tail);
            }
        }
        */

        /*
        //Output Receptor
        if(OutputTime!=0){
            if(j%OutputTimeStepsPerFrame==0){

                stringstream Receptor1;
                string Receptor2;

                Receptor1<<"Receptor"<<ModeT<<"-"<<int(j/OutputTimeStep)<<".txt";
                Receptor2=Receptor1.str();

                // output 3 print Receptor
                fstream output3;

                output3.open(Receptor2.c_str(), fstream::out | fstream::trunc);

                output3.precision(8);

                output3 << "coordX(1)\tcoordY(2)\t[B]\t[AB]\t#"<<endl;
                output3 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;

                for(int j=0;j<Rpy-1;j++){
                    for(int i=0;i<Rpx-1;i++){
                        int pointer=Rpx*(j)+(i);
                        output3 << ReceptorArray[pointer].coordX << "\t" << ReceptorArray[pointer].coordY << "\t" << ReceptorArray[pointer].B << "\t" << ReceptorArray[pointer].AB << endl;
                    }
                }

                output3.close();
            }
        }
        */

        //#pragma omp parallel for
        for(int i=0;i<DotN;i++){
            Analyte[i][0].flag=Analyte[i][1].flag;
            Analyte[i][0].coordX=Analyte[i][1].coordX;
            Analyte[i][0].coordY=Analyte[i][1].coordY;
            Analyte[i][0].coordZ=Analyte[i][1].coordZ;
        }

    //}while(j<TN && TotalCapture_now !=DotN);
    }while(TotalParticleOnWires_now < 100 && TotalCapture_now !=DotN);

    output1 << (TN-1)*tau << '\t'<< (TN-1) << '\t'<< TotalCapture_now << '\t' << CapturedOnSensor_now <<'\t'<< TotalCapture_now-CapturedOnSensor_now  <<'\t' << CapturedOnSensor_now/(double)TotalCapture_now<<'\t'<<((double)TotalCapture_now)/((double)MaximumCaptureNumberRSA2)<<endl;

    output4 << (TN-1)*tau << '\t'<< (TN-1) << '\t' << TotalParticleOnWires_now << '\t' ;
    for(int i=0;i<WireN;i++){
        output4 << ParticleOnWire[i] << '\t';
    }
    output4 << endl;

    MC_PrintTrojactory(-1);

    output4.close();
    output1.close();
}

void MonteCarlo::MC_DirectGenerateOnSurface(){


    int TotalCapture_now=0;
    int TotalCapture_previous=0;
    int CapturedOnSensor_now=0;

    int TotalParticleOnWires_now=0;
    int TotalParticleOnWires_previous=0;

    stringstream Statistic1;
    string Statistic2;

    Statistic1<<"Statistic"<<ModeT<<".txt";
    Statistic2=Statistic1.str();

    // output 1 print Statistic
    fstream output1;

    output1.open(Statistic2.c_str(), fstream::out | fstream::trunc);

    output1.precision(8);

    output1 << "T(1)\tN(2)\tAll(3)\tIn(4)\tOut(5)\tReff(6)\t #"<<endl;
    output1 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    output1 << 0*tau << '\t'<< 0 << '\t'<< TotalCapture_now << '\t' << CapturedOnSensor_now <<'\t'<< TotalCapture_now-CapturedOnSensor_now  <<'\t' << CapturedOnSensor_now/(double)TotalCapture_now<<'\t'<<endl;

    stringstream ParticleOnWire1;
    string ParticleOnWire2;

    ParticleOnWire1<<"ParticleOnWire"<<ModeT<<".txt";
    ParticleOnWire2=ParticleOnWire1.str();

    // output 4 print ParticleOnWire
    fstream output4;

    output4.open(ParticleOnWire2.c_str(), fstream::out | fstream::trunc);
    output4 << "T(1)\tN(2)\tTotal(3)\tTriggered(4)\tWire_1(5)\tWire_2(6)\tWire_3(7)\t #"<<endl;
    output4 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    output4 << 0*tau << '\t'<< 0 << '\t' << TotalParticleOnWires_now << '\t' ;
    for(int i=0;i<WireN;i++){
        output4 << ParticleOnWire[i] << '\t';
    }
    output4 << endl;


    // output 2 print ParticleOnWire
    stringstream ParticleOnWire11;
    string ParticleOnWire22;

    ParticleOnWire11<<"ParticleOnWireBox"<<ModeT<<".txt";
    ParticleOnWire22=ParticleOnWire11.str();
    fstream output2;


#pragma omp parallel for
    for(int i=0;i<DotN;i++){

        Analyte[i][0].coordX=lx/2-SensorWidth/2+rand() / (double)RAND_MAX*(SensorWidth); // initial coordinate
        Analyte[i][0].coordY=ly/2-SensorLength/2+rand() / (double)RAND_MAX*(SensorLength);

        Analyte[i][1].coordX=Analyte[i][0].coordX;
        Analyte[i][1].coordY=Analyte[i][0].coordY;
        Analyte[i][1].coordZ=0;

        //Stick
        MC_StickBoundary_onSensor(i,1);
    }


    //Reset Counter
    TotalCapture_now=0;
    CapturedOnSensor_now=0;
    TotalParticleOnWires_now=0;

    for(int i=0;i<WireN;i++){
        ParticleOnWire[i]=0;
    }

    //Counting
    for(int i=0;i<DotN;i++){
        if(Analyte[i][1].flag!=-1){
            TotalCapture_now=TotalCapture_now+1;

            //Rectangular Sensor
            if((Analyte[i][1].coordY>=ly/2-SensorLength/2)&&(Analyte[i][1].coordY<=ly/2+SensorLength/2)){
                if((Analyte[i][1].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][1].coordX<=lx/2+SensorWidth/2)){
                    //cout << Analyte[i][1].coordX <<" "<< Analyte[i][1].coordX<<" "<<lx/2-SensorWidth/2<<" "<<lx/2+SensorWidth/2<<" "<<ly/2-SensorLength/2<<" "<<ly/2+SensorLength/2<< endl;
                    CapturedOnSensor_now=CapturedOnSensor_now+1;
                }
            }

            //On wire
            for(int j=0;j<WireN;j++){
                if(abs(Analyte[i][1].coordX-(lx/2-SensorWidth/2)-(((WireWidth+WireSpacing)/2+j*WireWidth+WireSpacing)))< WireWidth/2){
                    TotalParticleOnWires_now=TotalParticleOnWires_now+1;
                    ParticleOnWire[j]=ParticleOnWire[j]+1;
                }
            }
        }
    }
    // Output when total capture particles is increased.
    if(TotalCapture_now!=TotalCapture_previous){
        output1 << 0 << '\t'<< 0 << '\t'<< TotalCapture_now << '\t' << CapturedOnSensor_now <<'\t'<< TotalCapture_now-CapturedOnSensor_now  <<'\t' << CapturedOnSensor_now/(double)TotalCapture_now<<'\t'<<((double)TotalCapture_now)/((double)MaximumCaptureNumberRSA2)<<endl;
    }

    TotalCapture_previous=TotalCapture_now;

    int triggered=0;
    for(int i=0;i<WireN;i++){
        if(ParticleOnWire[i]>0){
            triggered=triggered+1;
        }
    }

    // Output when total capture particles on wire is increased.
    if(TotalParticleOnWires_now!=TotalParticleOnWires_previous){
        output4 << 0 << '\t'<< 0 << '\t' << TotalParticleOnWires_now  << '\t'<<triggered<< '\t' ;
        for(int i=0;i<WireN;i++){
            output4 << ParticleOnWire[i] << '\t';
        }
        output4 << endl;
    }

    TotalParticleOnWires_previous=TotalParticleOnWires_now;


    output2.open(ParticleOnWire22.c_str(), fstream::out | fstream::trunc);
    output2 << "Wire N(1)\tN(2)\t #"<<endl;
    output2 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    for(int i=0;i<WireN;i++){
        output2 << i+1 <<'\t'<< ParticleOnWire[i] << endl;
    }

    output4.close();
    output2.close();
    output1.close();
}

int MonteCarlo::MC_ParticleTracing3DFindI(int dn, int tn){

    double Target=Analyte[dn][tn].coordX;
    int High=px-1;
    int Low=0;
    int Mid=0;
    int I=0;


    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            I=Low;
            break;

        }else{
            int pointer_Mid = (px)*(py)*(0) + (px)*(0) + (Mid);

            if(Target<=mesh[pointer_Mid].coordX){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return I;
}

int MonteCarlo::MC_ParticleTracing3DFindJ(int dn, int tn){

    double Target=Analyte[dn][tn].coordY;
    int High=py-1;
    int Low=0;
    int Mid=0;
    int J=0;


    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            J=Low;
            break;

        }else{
            int pointer_Mid = (px)*(py)*(0) + (px)*(Mid) + (0);

            if(Target<=mesh[pointer_Mid].coordY){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return J;
}

int MonteCarlo::MC_ParticleTracing3DFindJ_In2DField(int dn, int tn){

    double Target=Analyte[dn][tn].coordZ;
    int High=py-1;
    int Low=0;
    int Mid=0;
    int J=0;


    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            J=Low;
            break;

        }else{
            int pointer_Mid = (px)*(Mid) + (0);

            if(Target<=mesh[pointer_Mid].coordY){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return J;
}

int MonteCarlo::MC_ParticleTracing3DFindK(int dn, int tn){

    double Target=Analyte[dn][tn].coordZ;
    int High=pz-1;
    int Low=0;
    int Mid=0;
    int K=0;

    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            K=Low;
            break;

        }else{
            int pointer_Mid = (px)*(py)*(Mid) + (px)*(0) + (0);

            if(Target<=mesh[pointer_Mid].coordZ){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return K;
}

double MonteCarlo::MC_NormalDistribution(){

    //mean=expectation, sigma=standard deviation.

    //Box-Muller
    //RAND_MAX=2147483647 31-bit


    double u, v;//uniform distribution
    double x;//, y;//normal distribution

    u = rand() / (double)(RAND_MAX); // random double from 0 to 1.
    v = rand() / (double)(RAND_MAX);

    x = sqrt(-2 * log(u)) * cos(2 * M_PI * v) * sigma + mean;//M_PI=3.14159
    //y = sqrt(-2 * log(u)) * sin(2 * M_PI * v) * sigma + mean;

    return x;

}

void MonteCarlo::MC_StickBoundary_onSensor(int i, int j){

    //Absorb all particle who reach the sensor area.

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=0){

        int tag=0;
        double z1=abs(Analyte[i][j].coordZ-0);
        double z2=abs(Analyte[i][j-1].coordZ-0);
        double Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
        double Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

        if((Xtouch>lx/2-SensorWidth/2)&&(Xtouch<lx/2+SensorWidth/2)){
            if((Ytouch>ly/2-SensorLength/2)&&(Ytouch<ly/2+SensorLength/2)){

                Analyte[i][j].coordX=Xtouch; // overwrite previous result
                Analyte[i][j].coordY=Ytouch;
                Analyte[i][j].coordZ=0;
                Analyte[i][j].flag=1;
                tag=1;
            }
        }

        if(tag==0){
            Analyte[i][j].coordZ=0+abs(0-Analyte[i][j].coordZ);
        }

    }
    //else no change
}

void MonteCarlo::MC_RSABoundary_onSensor(int i, int j){

    //Absorb all particle who reach the sensor area.
    //considing the molecule radius overlap
    //RSA

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=0){

        double z1=abs(Analyte[i][j].coordZ-0);
        double z2=abs(Analyte[i][j-1].coordZ-0);
        double Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
        double Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

        if((Xtouch>=lx/2-SensorWidth/2 && Xtouch<=lx/2+SensorWidth/2)&&(Ytouch>=ly/2-SensorLength/2 && Ytouch<=ly/2+SensorLength/2)){

            int CapturePermission=1; //permitted=1 prohibit=0;

            for(int m=0;m<DotN;m++){
                if(m!=i){
                    if(Analyte[m][j].flag!=-1){
                        if(pow(Analyte[m][j].coordX-Xtouch,2)+pow(Analyte[m][j].coordY-Ytouch,2)<pow(2*AnalyteRadius_nm,2)){
                             CapturePermission=0;
                        }
                    }
                }
            }

            if(CapturePermission==1){
                Analyte[i][j].coordX=Xtouch; // overwrite previous result
                Analyte[i][j].coordY=Ytouch;
                Analyte[i][j].coordZ=0;
                Analyte[i][j].flag=1;
                //cout << "capture !, j="<<j<<" m="<<Analyte[i][j].flag<<" ,i="<<ReceptorArray[receptor_flag].flag<<" "<<Analyte[i][j].coordX<<" "<<Analyte[i][j].coordY<<" "<<Analyte[i][j-1].coordY<<" "<<Analyte[i][j-2].flag<< endl;
            }else{
                Analyte[i][j].coordZ=0+(0-Analyte[i][j].coordZ);
            }
        }else{
            Analyte[i][j].coordZ=0+(0-Analyte[i][j].coordZ);
        }
    }
    //else no change
}

void MonteCarlo::MC_StickBoundary_BottomSurface(int i, int j){

    //Absorb all particle who reach the BottomSurface.

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=0){

        double z1=abs(Analyte[i][j].coordZ-0);
        double z2=abs(Analyte[i][j-1].coordZ-0);
        double Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
        double Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

        Analyte[i][j].coordX=Xtouch; // overwrite previous result
        Analyte[i][j].coordY=Ytouch;
        Analyte[i][j].coordZ=0;
        Analyte[i][j].flag=1;
    }
    //else no change
}

void MonteCarlo::MC_RSABoundary_BottomSurface(int i, int j){

    //absorb all particle who reach the BottomSurface.
    //considing the molecule radius overlap
    //RSA

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=0){

        double z1=abs(Analyte[i][j].coordZ-0);
        double z2=abs(Analyte[i][j-1].coordZ-0);
        double Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
        double Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

        int CapturePermission=1; //permitted=1 prohibit=0;

        for(int m=0;m<DotN;m++){
            if(m!=i){
                if(Analyte[m][j].flag!=-1){
                    if(pow(Analyte[m][j].coordX-Xtouch,2)+pow(Analyte[m][j].coordY-Ytouch,2)<pow(2*AnalyteRadius_nm,2)){
                         CapturePermission=0;
                    }
                }
            }
        }

        if(CapturePermission==1){
            Analyte[i][j].coordX=Xtouch; // overwrite previous result
            Analyte[i][j].coordY=Ytouch;
            Analyte[i][j].coordZ=0;
            Analyte[i][j].flag=1;
            //cout << "capture !, j="<<j<<" m="<<Analyte[i][j].flag<<" ,i="<<ReceptorArray[receptor_flag].flag<<" "<<Analyte[i][j].coordX<<" "<<Analyte[i][j].coordY<<" "<<Analyte[i][j-1].coordY<<" "<<Analyte[i][j-2].flag<< endl;
        }else{
            Analyte[i][j].coordZ=0+(0-Analyte[i][j].coordZ);
        }
    }
    //else no change
}

void MonteCarlo::MC_FirstOrderBoundary(int i, int j){

    //Bottom BC absorb particle according to first order chemical reaction

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=ReceptorMesh){

        int tag=0;
        double Xtouch=0;
        double Ytouch=0;
        if(Analyte[i][j].coordZ<0){
            double z1=abs(Analyte[i][j].coordZ-0);
            double z2=abs(Analyte[i][j-1].coordZ-0);
            Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
            Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

            Analyte[i][j].coordX=Xtouch; // overwrite previous result
            Analyte[i][j].coordY=Ytouch;
            Analyte[i][j].coordZ=0;
        }

        if((Analyte[i][1].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][1].coordX<=lx/2+SensorWidth/2)){
            if((Analyte[i][1].coordY>=ly/2-SensorLength/2)&&(Analyte[i][1].coordY<=ly/2+SensorLength/2)){

                int I=0;
                int J=0;
                //Find I
                I=MC_BCFindI(i,j);
                //Find J
                J=MC_BCFindJ(i,j);
                //Calculate pointer
                int pointer=Rpx*(J)+(I);

                //Reaction
                //R1=k1[A][B]
                //     MA/s = 1/(M*s) * M  * MA
                double R1  =k_forward*LC_A*ReceptorArray[pointer].B;

                //     MA   = MA/s * s
                double R1MA = R1   * tau;

                double R1N = R1MA*Avogadro*(ReceptorMesh*1e-9)*(ReceptorMesh*1e-9);


                if (R1N>1){
                    //cout << R1N << endl;
                    Analyte[i][j].coordZ=0;
                    Analyte[i][j].flag=1;
                    tag=1;
                    ReceptorArray[pointer].B=ReceptorArray[pointer].B-UnitCon;
                    ReceptorArray[pointer].AB=ReceptorArray[pointer].AB+UnitCon;
                }else{
                    if(rand()/(double)RAND_MAX < R1N){
                        Analyte[i][j].coordZ=0;
                        Analyte[i][j].flag=1;
                        tag=1;
                        //cerr <<"I="<<I<<" J="<<J<<" pointer="<<pointer<<" ";cout << ReceptorArray[pointer].B<<endl;
                        ReceptorArray[pointer].B=ReceptorArray[pointer].B-UnitCon;
                        ReceptorArray[pointer].AB=ReceptorArray[pointer].AB+UnitCon;
                    }
                    //else no change
                }
            }
        }

        if(tag==0){
            if(Analyte[i][j].coordZ<0){
                Analyte[i][j].coordZ=0+abs(0-Analyte[i][j].coordZ);
            }
            //else no change
        }
    }
    //else no change
}

void MonteCarlo::MC_FirstOrderBoundary_FiniteR(int i, int j){

    //Each receptor block can only receive one molecules

    //Bottom BC absorb particle according to first order chemical reaction

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=2*AnalyteRadius_nm){

        double Xtouch=0;
        double Ytouch=0;
        if(Analyte[i][j].coordZ<0){
            double z1=abs(Analyte[i][j].coordZ-0);
            double z2=abs(Analyte[i][j-1].coordZ-0);
            Xtouch=Analyte[i][j-1].coordX+(Analyte[i][j].coordX-Analyte[i][j-1].coordX)*(z2/(z1+z2));
            Ytouch=Analyte[i][j-1].coordY+(Analyte[i][j].coordY-Analyte[i][j-1].coordY)*(z2/(z1+z2));

            Analyte[i][j].coordX=Xtouch; // overwrite previous result
            Analyte[i][j].coordY=Ytouch;
            Analyte[i][j].coordZ=0;
        }

        int tag=0;
        if((Analyte[i][1].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][1].coordX<=lx/2+SensorWidth/2)){
            if((Analyte[i][1].coordY>=ly/2-SensorLength/2)&&(Analyte[i][1].coordY<=ly/2+SensorLength/2)){


                int I=0;
                int J=0;
                //Find I
                I=MC_BCFindI(i,j);
                //Find J
                J=MC_BCFindJ(i,j);
                //Calculate pointer
                int pointer=Rpx*(J)+(I);

                //Reaction
                //R1=k1[A][B]
                //     MA/s = 1/(M*s) * M  * MA
                double R1  =k_forward*LC_A*ReceptorArray[pointer].B;

                //     MA   = MA/s * s
                double R1MA = R1   * tau;

                double R1N = R1MA*Avogadro*(ReceptorMesh*1e-9)*(ReceptorMesh*1e-9);


                if (R1N>1){
                    //cout << R1N << endl;
                    Analyte[i][j].coordZ=0;
                    Analyte[i][j].flag=1;
                    tag=1;
                    ReceptorArray[pointer].B=0;
                    ReceptorArray[pointer].AB=ReceptorArray[pointer].AB+UnitCon;
                }else{
                    if(rand()/(double)RAND_MAX < R1N){
                        Analyte[i][j].coordZ=0;
                        Analyte[i][j].flag=1;
                        tag=1;
                        //cerr <<"I="<<I<<" J="<<J<<" pointer="<<pointer<<" ";cout << ReceptorArray[pointer].B<<endl;
                        ReceptorArray[pointer].B=0;
                        ReceptorArray[pointer].AB=ReceptorArray[pointer].AB+UnitCon;
                    }
                    //else no change
                }
            }
        }

        if(tag==0){
            if(Analyte[i][j].coordZ<0){
                Analyte[i][j].coordZ=0+abs(0-Analyte[i][j].coordZ);
            }
            //else no change
        }
    }
    //else no change
}

void MonteCarlo::MC_FirstOrderBoundary_RSA_onSensor_BlockReceptor(int i, int j){

    //Each receptor block can only receive one molecules

    //Bottom BC absorb particle according to first order chemical reaction

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=ReceptorMesh){

        int tag=0;

        //Change coordZ to 0
        if(Analyte[i][j].coordZ<0){
            Analyte[i][j].coordZ=0;
        }

        //Check capture_enable
        if((Analyte[i][j].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][j].coordX<=lx/2+SensorWidth/2)){
            if((Analyte[i][j].coordY>=ly/2-SensorLength/2)&&(Analyte[i][j].coordY<=ly/2+SensorLength/2)){

                int capture_enable=1; //enable=1 disable=0;

                for(int m=0;m<DotN;m++){
                    if(Analyte[m][j].flag!=-1){
                        if(m!=i){
                            if(pow(Analyte[m][j].coordX-Analyte[i][j].coordX,2)+pow(Analyte[m][j].coordY-Analyte[i][j].coordY,2)<pow(2*AnalyteRadius_nm,2)){
                                 capture_enable=0;
                            }
                        }
                    }
                }

                //if RSA capture is permitted
                if (capture_enable==1){
                    int I=0;
                    int J=0;
                    //Find I
                    I=MC_BCFindI(i,j);
                    //Find J
                    J=MC_BCFindJ(i,j);
                    //Calculate pointer
                    int pointer=Rpx*(J)+(I);

                    //Reaction
                    //R1=k1[A][B]
                    //     MA/s = 1/(M*s) * M  * MA
                    double R1  =k_forward*LC_A*ReceptorArray[pointer].B;

                    //     MA   = MA/s * s
                    double R1MA = R1   * tau;

                    double R1N = R1MA*Avogadro*(ReceptorMesh*1e-9)*(ReceptorMesh*1e-9);

                    if(rand()/(double)RAND_MAX < R1N){
                        Analyte[i][j].coordZ=0;
                        Analyte[i][j].flag=1;
                        tag=1;
                        //cerr <<"I="<<I<<" J="<<J<<" pointer="<<pointer<<" ";cout << ReceptorArray[pointer].B<<endl;
                        ReceptorArray[pointer].B=0;
                        ReceptorArray[pointer].AB=ReceptorArray[pointer].AB+UnitCon;
                    }
                }
            }
        }

        if(tag==0){
            if(Analyte[i][j].coordZ<0){
                Analyte[i][j].coordZ=0+abs(0-Analyte[i][j].coordZ);
            }
            //else no change
        }
    }
    //else no change

}

void MonteCarlo::MC_FirstOrderBoundary_RSA_onSensor_Cylindrical(int i, int j){

    //RSA without using receptor blocks, using cylindrical unit volume.

    //Bottom BC absorb particle according to first order chemical reaction

    // coordX
    if(Analyte[i][j].coordX>=lx){
        Analyte[i][j].coordX=lx-abs(Analyte[i][j].coordX-lx);
    }else if(Analyte[i][j].coordX<=0){
        Analyte[i][j].coordX=0+abs(0-Analyte[i][j].coordX);
    }else{
        Analyte[i][j].coordX=Analyte[i][j].coordX;
    }

    // coordY
    if(Analyte[i][j].coordY>=ly){
        Analyte[i][j].coordY=ly-abs(Analyte[i][j].coordY-ly);
    }else if(Analyte[i][j].coordY<=0){
        Analyte[i][j].coordY=0+abs(0-Analyte[i][j].coordY);
    }else{
        Analyte[i][j].coordY=Analyte[i][j].coordY;
    }

    // coordZ
    if(Analyte[i][j].coordZ>=lz){
        Analyte[i][j].coordZ=lz-(Analyte[i][j].coordZ-lz);
    }else if(Analyte[i][j].coordZ<=ReceptorMesh){

        int tag=0;

        //Change coordZ to 0
        if(Analyte[i][j].coordZ<0){
            Analyte[i][j].coordZ=0;
        }

        //Check capture_enable
        if((Analyte[i][j].coordX>=lx/2-SensorWidth/2)&&(Analyte[i][j].coordX<=lx/2+SensorWidth/2)){
            if((Analyte[i][j].coordY>=ly/2-SensorLength/2)&&(Analyte[i][j].coordY<=ly/2+SensorLength/2)){

                int capture_enable=1; //enable=1 disable=0;

                for(int m=0;m<DotN;m++){
                    if(Analyte[m][j].flag!=-1){
                        if(m!=i){
                            if(pow(Analyte[m][j].coordX-Analyte[i][j].coordX,2)+pow(Analyte[m][j].coordY-Analyte[i][j].coordY,2)<pow(2*AnalyteRadius_nm,2)){
                                 capture_enable=0;
                            }
                        }
                    }
                }

                //if RSA capture is permitted
                if ((capture_enable==1)&&(rand()/(double)RAND_MAX < R1N_cylindrical)){
                    Analyte[i][j].coordZ=0;
                    Analyte[i][j].flag=1;
                    tag=1;
                }
            }
        }

        if(tag==0){
            if(Analyte[i][j].coordZ<0){
                Analyte[i][j].coordZ=0+abs(0-Analyte[i][j].coordZ);
            }
            //else no change
        }
    }
    //else no change

}

int MonteCarlo::MC_BCFindI(int dn, int tn){

    double Target=Analyte[dn][tn].coordX;
    int High=Rpx-1;
    int Low=0;
    int Mid=0;
    int I=0;


    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            I=Low;

            //int pointer_Mid = (Rpx)*(0) + (Mid);
            //cout <<"Low="<<Low<<" Mid="<<Mid<<" High="<<High<<" Target="<<Target<<" ReceptorArray[pointer_Mid].coordX="<<ReceptorArray[pointer_Mid].coordX<<endl;
            break;

        }else{
            int pointer_Mid = (Rpx)*(0) + (Mid);

            if(Target<=ReceptorArray[pointer_Mid].coordX-ReceptorMesh/2){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return I;
}

int MonteCarlo::MC_BCFindJ(int dn, int tn){

    double Target=Analyte[dn][tn].coordY;
    int High=Rpy-1;
    int Low=0;
    int Mid=0;
    int J=0;


    do{

        Mid=(High+Low)/2;

        if(Low==Mid){
            J=Low;
            break;

        }else{
            int pointer_Mid = (Rpx)*(Mid) + (0);

            if(Target<=ReceptorArray[pointer_Mid].coordY-ReceptorMesh/2){
                High=Mid;
            }else{
                Low=Mid;
            }
        }

    }while(1);

    return J;
}

void MonteCarlo::MC_FirstOrderRelease(int i, int j){

    int I=0;
    int J=0;
    //Find I
    I=MC_BCFindI(i,j);
    //Find J
    J=MC_BCFindJ(i,j);
    //Calculate pointer
    int pointer=Rpx*(J)+(I);

    Analyte[i][j].flag=-1;
    ReceptorArray[pointer].B=ReceptorArray[pointer].B+UnitCon;
    ReceptorArray[pointer].AB=ReceptorArray[pointer].AB-UnitCon;
}

void MonteCarlo::MC_FirstOrderRelease_FiniteR(int i, int j){

    int I=0;
    int J=0;
    //Find I
    I=MC_BCFindI(i,j);
    //Find J
    J=MC_BCFindJ(i,j);
    //Calculate pointer
    int pointer=Rpx*(J)+(I);

    Analyte[i][j].flag=-1;
    ReceptorArray[pointer].B=C_B;
    ReceptorArray[pointer].AB=ReceptorArray[pointer].AB-UnitCon;
}

double MonteCarlo::MC_factorial(double n){

    if (n<=1){
        return 1;
    }else{
        return n*MC_factorial(n-1);;
    }
}

double MonteCarlo::MC_Ccomb(double m, double n){

    double A = MC_factorial(m);
    double B = MC_factorial(n);
    double C = MC_factorial(m-n);

    return A/B/C;
}

double MonteCarlo::MC_Pbinominal(double m, double n, double p){

    return MC_Ccomb(m,n)*pow(p,n)*pow(1-p,m-n);
}

double MonteCarlo::MC_Hcomb(double m, double n){

    return MC_Ccomb(m+n-1,n);
}

void MonteCarlo::MC_Distribution(){

    fstream output1,output2,output3;

    output1.open("Binominal.txt", fstream::out | fstream::trunc);
    output2.open("Nanowire.txt", fstream::out | fstream::trunc);
    output3.open("DistributionParameter.txt", fstream::out | fstream::trunc);

    double p=0.2;
    double total_nanowire_number=100;
    double total_biomolecues=120;

    output3<<"Biosensor Control Method Parameter"<<endl;
    output3<<"Total Biomolecules="<<total_biomolecues<<endl;
    output3<<"Biomolecules Capture Probability p="<<p<<endl;
    output3<<"Total Nanowires="<<total_nanowire_number<<endl<<endl;

    output1 << "N" <<'\t'<< "P_bi" <<'\t'<<endl;
    output2 << "N" <<'\t'<< "P_wire" <<'\t'<<endl;

    double *BProb= new double [(int)total_biomolecues];
    for(int i=0;i<total_biomolecues;i++){
        BProb[i]=0;
    }

    //Calculate binominal distribution
    double check1=0;
    double AvgBiomolecules=0;
    for(int i=0;i<total_biomolecues+1;i++){
        check1=check1+MC_Pbinominal(total_biomolecues,i,p);
        BProb[i]=MC_Pbinominal(total_biomolecues,i,p);
        AvgBiomolecules=AvgBiomolecules+i*BProb[i];
        output1 <<scientific<< i <<'\t'<<BProb[i]<<endl;
    }
    cout << "Total Prob1="<<check1<<endl;

    output3<<"Average Captured Biomolecules="<<AvgBiomolecules<<endl;

    double *NProb= new double [(int)total_biomolecues];
    for(int i=0;i<total_biomolecues;i++){
        NProb[i]=0;
    }

    //Calculate Nanowire distribution
    for(int i=0;i<total_biomolecues+1;i++){ // captured biomolecules
        for(int j=0;j<i+1;j++){ // triggered nanowires

            NProb[j]=NProb[j]+MC_Pbinominal(total_biomolecues,i,p)*MC_Ccomb(total_nanowire_number,j)*MC_Hcomb(j,i-j)/MC_Hcomb(total_nanowire_number,i);
        }
    }

    double check2=0;
    double AvgNanowires=0;
    for(int i=0;i<total_biomolecues+1;i++){
        check2=check2+NProb[i];
        AvgNanowires=AvgNanowires+NProb[i]*i;
        output2 << i << "\t" << NProb[i]<<endl;

    }
    cout << "Total Prob2="<<check2<<endl;

    output3<<"Average Triggered Nanowires="<<AvgNanowires<<endl;

    double BVariance=0;
    //Calculate standard deviation
    for(int i=0;i<total_biomolecues+1;i++){
        BVariance=BVariance+pow(i,2)*BProb[i];
    }
    BVariance=BVariance-pow(AvgBiomolecules,2);

    double Bdeviation=sqrt(BVariance);

    output3<<"Biomolecules standard deviation="<<Bdeviation<<endl;


    double NVariance=0;
    //Calculate standard deviation
    for(int i=0;i<total_biomolecues+1;i++){
        NVariance=NVariance+pow(i,2)*NProb[i];
    }
    NVariance=NVariance-pow(AvgNanowires,2);

    double Ndeviation=sqrt(NVariance);

    output3<<"Biomolecules standard deviation="<<Ndeviation<<endl;

    output1.close();
    output2.close();
}

void MonteCarlo::MC_PrintTrojactory(int tail){

    stringstream Traj1;
    string Traj2;

    Traj1<<"Traj"<<ModeT<<"-"<<tail<<".txt";
    Traj2=Traj1.str();

    // output 2 print Trajactory
    fstream output2;

    output2.open(Traj2.c_str(), fstream::out | fstream::trunc);

    output2.precision(8);

    output2 << "N(1)\t";
    for(int i=0;i<3;i++){
        output2 << "coordX("<<i*4+2<<")("<<i<<")\tcoordY("<<i*4+3<<")("<<i<<")\tcoordZ("<<i*4+4<<")("<<i<<")\tflag("<<i*4+5<<")\t";
    }
    output2 << "#"<<endl;
    output2 <<"--------------------------------------------------------------------------------------------------------------------------------#" << endl;
    output2 << 0 << '\t';
    for(int i=0;i<OutputMoleculeNumber;i++){
        output2 << Analyte[i][0].coordX << '\t' <<  Analyte[i][0].coordY << '\t' << Analyte[i][0].coordZ << '\t' << Analyte[i][0].flag << '\t';
    }
        output2 << endl ;

    output2.close();
}



