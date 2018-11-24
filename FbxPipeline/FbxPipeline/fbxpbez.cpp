#include <dlib/optimization.h>
#include <fbxpstate.h>

namespace BezierFitter {
    using namespace dlib;

    // dlib reference fo the solver:
    // http://dlib.net/least_squares_ex.cpp.html

    typedef matrix< double, 3, 1 >                 BezierFitterInput;  // double t, P0, P3;
    typedef matrix< double, 2, 1 >                 BezierFitterParams; // double P1, P2;
    typedef std::pair< BezierFitterInput, double > BezierFitterSample; // double t, P0, P3 + Bt;

    inline double Squared( double v ) {
        return v * v;
    }

    inline double Cubed( double v ) {
        return v * v * v;
    }

    inline double BezierFitterModel( const BezierFitterInput input, const BezierFitterParams params ) {
        const double t  = input( 0 );
        const double P0 = input( 1 );
        const double P1 = params( 0 );
        const double P2 = params( 1 );
        const double P3 = input( 2 );
        return Cubed( 1.0 - t ) * P0 + 3 * Squared( 1.0 - t ) * t * P1 + 3 * ( 1.0 - t ) * Squared( t ) * P2 + Cubed( t ) * P3;
    }

    inline double BezierFitterResidual( const BezierFitterSample input, const BezierFitterParams params ) {
        const double Bt = input.second;
        return BezierFitterModel( input.first, params ) - Bt;
    }

    inline BezierFitterParams BezierFitterResidualDerivative( const BezierFitterSample input,
                                                              const BezierFitterParams params ) {
        const double       t = input.first( 0 );
        BezierFitterParams derivative;
        derivative( 0 ) = 3 * Squared( 1.0 - t ) * t;
        derivative( 1 ) = 3 * ( 1.0 - t ) * Squared( t );
        return derivative;
    }

    static BezierFitterParams SolveBezier( std::vector< BezierFitterSample > samples ) {
        BezierFitterParams bezierSolverParams;
        bezierSolverParams = 0;

        // Use the Levenberg-Marquardt method to determine the parameters which
        // minimize the sum of all squared residuals.
        solve_least_squares_lm( objective_delta_stop_strategy( 1e-7 ),
                                BezierFitterResidual,
                                BezierFitterResidualDerivative,
                                samples,
                                bezierSolverParams );

        // If we didn't create the residual_derivative function then we could
        // have used this method which numerically approximates the derivatives for you.
        // solve_least_squares_lm( objective_delta_stop_strategy( 1e-7 ).be_verbose( ),
        //                         BezierFitterResidual,
        //                         derivative( BezierFitterResidual ),
        //                         samples,
        //                         x );

        // This version of the solver uses a method which is appropriate for problems
        // where the residuals don't go to zero at the solution.  So in these cases
        // it may provide a better answer.
        // solve_least_squares( objective_delta_stop_strategy( 1e-7 ).be_verbose( ),
        // solve_least_squares( objective_delta_stop_strategy( 1e-7 ),
        //                      BezierFitterResidual,
        //                      BezierFitterResidualDerivative,
        //                      samples,
        //                      bezierSolverParams );

        return bezierSolverParams;
    }

    static void ExtractSamples( FbxAnimCurve*                      pAnimCurve,
                                const int                          /*startIndex*/,
                                const double                       P0X,
                                const double                       P0Y,
                                const double                       P3X,
                                const double                       P3Y,
                                std::vector< BezierFitterSample >& samples ) {

        // TODO: Scan only the relevan region, break the loop when the end time is reached.
        for ( int i = 0; i < pAnimCurve->KeyGetCount(); ++i ) {
            const double time = pAnimCurve->KeyGetTime( i ).GetSecondDouble( );

            if ( time >= P0X && time <= P3X ) {
                const double t  = ( time - P0X ) / ( P3X - P0X );
                const double Bt = pAnimCurve->KeyGetValue( i );

                BezierFitterSample sample;
                sample.first( 0 ) = t;
                sample.first( 1 ) = P0Y;
                sample.first( 2 ) = P3Y;
                sample.second     = Bt;

                samples.push_back( sample );
            }
        }
    }

} // namespace BezierFitter

bool BezierFitterFitSamples( FbxAnimCurve* pAnimCurve,
                             const int     startIndex,
                             const double  BezP0X,
                             const double  BezP0Y,
                             const double  BezP3X,
                             const double  BezP3Y,
                             double&       BezP1Y,
                             double&       BezP2Y ) {
    std::vector< BezierFitter::BezierFitterSample > samples;
    BezierFitter::ExtractSamples( pAnimCurve, startIndex, BezP0X, BezP0Y, BezP3X, BezP3Y, samples );

    auto & s = apemode::State::Get( );
    if ( !samples.empty( ) ) {
        auto params = BezierFitter::SolveBezier( std::move( samples ) );
        BezP1Y = params( 0 );
        BezP2Y = params( 1 );
        s.console->debug( "Samples taken: {}", samples.size( ) );
        s.console->debug( "Solved Bezier: {} {}", BezP1Y, BezP2Y );
        return true;
    }

    s.console->error("Failed to find the samples for fitting the Bezier control points.");
    return false;
}

void BezierFitterFitSamples( FbxAnimCurve* pAnimCurve, int keyIndex, double& OutFittedBezier1, double& OutFittedBezier2 ) {
    assert( pAnimCurve && ( keyIndex < ( pAnimCurve->KeyGetCount( ) - 1 ) ) );
    auto& s = apemode::State::Get( );

    const FbxString copiedCurveName = pAnimCurve->GetNameOnly() + " [FbxPipeline-Copy]";
    if ( FbxAnimCurve* pCopiedAnimCurve = FbxAnimCurve::Create( s.manager, copiedCurveName.Buffer() ) ) {
        pCopiedAnimCurve->CopyFrom( *pAnimCurve );

        auto resampleStartTime = pAnimCurve->KeyGet( keyIndex ).GetTime( );
        auto resampleStopTime  = pAnimCurve->KeyGet( keyIndex + 1 ).GetTime( );

        FbxTime resamplePeriodTime;
        resamplePeriodTime.SetMilliSeconds( ( FbxLongLong )( 1000.0f / 180.0f ) );

        FbxAnimCurveFilterResample animCurveFilterResample;
        animCurveFilterResample.SetPeriodTime( resamplePeriodTime );
        animCurveFilterResample.SetStartTime( resampleStartTime );
        animCurveFilterResample.SetStopTime( resampleStopTime );
        animCurveFilterResample.Apply( *pCopiedAnimCurve );

        BezierFitterFitSamples( pCopiedAnimCurve,
                                keyIndex,
                                pAnimCurve->KeyGet( keyIndex ).GetTime( ).GetSecondDouble( ),
                                pAnimCurve->KeyGet( keyIndex ).GetValue( ),
                                pAnimCurve->KeyGet( keyIndex + 1 ).GetTime( ).GetSecondDouble( ),
                                pAnimCurve->KeyGet( keyIndex + 1 ).GetValue( ),
                                OutFittedBezier1,
                                OutFittedBezier2 );

        pCopiedAnimCurve->Destroy( );
    }
}
