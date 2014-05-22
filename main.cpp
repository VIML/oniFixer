// STL Header
#include <array>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// OpenNI Header
#include <OpenNI.h>
#include <PS1080.h>

using namespace std;
using namespace openni;

#define GET_VIRTUAL_STREAM_IMAGE	100000
#define SET_VIRTUAL_STREAM_IMAGE	100001

// the NewFrameListener
class CFrameCopy : public VideoStream::NewFrameListener
{
public:
	CFrameCopy( VideoStream& rStream ) : m_rVStream( rStream )
	{
	}

	void onNewFrame( openni::VideoStream& rStream )
	{
		openni::VideoFrameRef mFrame;
		// read frame from real video stream
		if( rStream.readFrame( &mFrame ) == openni::STATUS_OK )
		{
			// get a frame form virtual video stream
			OniFrame* pFrame = NULL;
			if( m_rVStream.invoke( GET_VIRTUAL_STREAM_IMAGE, pFrame ) == openni::STATUS_OK )
			{
				memcpy( pFrame->data, mFrame.getData(), mFrame.getDataSize() );

				pFrame->frameIndex = mFrame.getFrameIndex();
				pFrame->timestamp = mFrame.getTimestamp();

				// write data to form virtual video stream
				m_rVStream.invoke( SET_VIRTUAL_STREAM_IMAGE, pFrame );
			}
		}
	}

protected:
	openni::VideoStream&	m_rVStream;

	CFrameCopy& operator=(const CFrameCopy&);
};

// The class used to process the properties that NiTE require
class CNiTEProperty
{
public:
	CNiTEProperty()
	{
		size_t	i = 0;
		vProperties[  i] = CProperty( XN_STREAM_PROPERTY_CONST_SHIFT			, 8		, "XN_STREAM_PROPERTY_CONST_SHIFT"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_PARAM_COEFF			, 8		, "XN_STREAM_PROPERTY_PARAM_COEFF"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_SHIFT_SCALE			, 8		, "XN_STREAM_PROPERTY_SHIFT_SCALE"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_MAX_SHIFT				, 8		, "XN_STREAM_PROPERTY_MAX_SHIFT"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_S2D_TABLE				, 4096	, "XN_STREAM_PROPERTY_S2D_TABLE"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_D2S_TABLE				, 20002	, "XN_STREAM_PROPERTY_D2S_TABLE"				);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE	, 8		, "XN_STREAM_PROPERTY_ZERO_PLANE_DISTANCE"		);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE	, 8		, "XN_STREAM_PROPERTY_ZERO_PLANE_PIXEL_SIZE"	);
		vProperties[++i] = CProperty( XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE	, 8		, "XN_STREAM_PROPERTY_EMITTER_DCMOS_DISTANCE"	);
	}

	bool LoadProperties( const VideoStream& rStream, bool bOverwrite = false )
	{
		for_each( vProperties.begin(), vProperties.end(), [&rStream,&bOverwrite]( CProperty& rProp ){
			if( bOverwrite || !rProp.bLoad )
			{
				cout << "Try load " << rProp.sName << "\t-> ";
				int iSize = rProp.vData.size();
				if( rStream.getProperty( rProp.iIdx, rProp.vData.data(), &iSize ) == STATUS_OK )
				{
					rProp.bLoad = true;
					cout << "OK";
				}
				else
				{
					cout << "Fail";
				}
				cout << endl;
			}
		} );

		return all_of( vProperties.begin(), vProperties.end(), []( CProperty& rProp ){
			return rProp.bLoad;
		} );
	}

	void WriteProperties( VideoStream& rStream )
	{
		for_each( vProperties.begin(), vProperties.end(), [&rStream]( CProperty& rProp ){
			int iSize = rProp.vData.size();
			if( rStream.setProperty( rProp.iIdx, rProp.vData.data(), iSize ) != STATUS_OK )
			{
				cerr << "Property " << rProp.sName << " write fail" << endl;
			}
		} );
	}

protected:
	class CProperty
	{
	public:
		CProperty(){};
		CProperty( int idx, int size, string name )
		{
			iIdx	= idx;
			sName	= name;
			bLoad	= false;
			vData.resize(size);
		}
	
		int		iIdx;
		string	sName;
		bool	bLoad;
		std::vector<char>	vData;
	};

protected:
	array<CProperty,9>	vProperties;
};

int main( int argc, char** argv )
{
	std::string	sSource, sTarget;
	#pragma region check input parameters
	if( argc == 3 )
	{
		sSource = argv[1];
		sTarget = argv[2];
	}
	else
	{
		cerr << "Usage: oniFixer source.oni target.oni\n" << endl;
	}
	#pragma endregion

	// Initial OpenNI 
	if( OpenNI::initialize() != STATUS_OK )
	{
		cerr << "OpenNI Initial Error: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	CNiTEProperty	mNiTEProp;

	#pragma region Open and check source file
	// Open source file
	cout << "Open the ONI file: " << sSource << endl;
	Device	devSourceFile;
	if( devSourceFile.open( sSource.c_str() ) != STATUS_OK )
	{
		cerr << "Can't open source file: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// Create depth stream
	VideoStream vsSourceDepth;
	if( devSourceFile.hasSensor( SENSOR_DEPTH ) )
	{
		if( vsSourceDepth.create( devSourceFile, SENSOR_DEPTH ) != STATUS_OK )
		{
			cerr << "Can't create depth stream: " << OpenNI::getExtendedError() << endl;
			return -1;
		}
	}
	else
	{
		cerr << "ERROR: This source file does not have depth sensor" << endl;
		return -1;
	}

	// check NiTE required property
	if( mNiTEProp.LoadProperties( vsSourceDepth ) )
	{
		cout << "All required properties works, don't need to fix" << endl;
		return 0;
	}
	#pragma endregion

	#pragma region Create a physical device to read property
	cout << "\nOpen a physical device to read property" << endl;

	Device devRealDevice;
	if( devRealDevice.open( ANY_DEVICE ) != STATUS_OK )
	{
		cerr << "Can't open real device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	VideoStream vsRealDepth;
	if( devRealDevice.hasSensor( SENSOR_DEPTH ) )
	{
		if( vsRealDepth.create( devRealDevice, SENSOR_DEPTH ) != STATUS_OK )
		{
			cerr << "Can't create depth stream: " << OpenNI::getExtendedError() << endl;
			return -1;
		}
	}
	else
	{
		cerr << "ERROR: This source file does not have depth sensor" << endl;
		return -1;
	}

	// try to load property
	if( !mNiTEProp.LoadProperties( vsRealDepth ) )
	{
		cout << "Can't prepare all required properties" << endl;
		return -1;
	}

	vsRealDepth.destroy();
	devRealDevice.close();
	#pragma endregion

	#pragma region Create a virtual device to record
	cout << "\nCreate a virtual device to record" << endl;

	// create virtual device
	Device devVirDevice;
	if( devVirDevice.open( "\\OpenNI2\\VirtualDevice\\PS1080" ) != STATUS_OK )
	{
		cerr << "Can't open virtual device: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// create virtual depth stream
	VideoStream vsVirDepth;
	if( vsVirDepth.create( devVirDevice, SENSOR_DEPTH ) != STATUS_OK )
	{
		cerr << "Can't create depth stream: " << OpenNI::getExtendedError() << endl;
		return -1;
	}

	// assign basic properties
	vsVirDepth.setProperty( ONI_STREAM_PROPERTY_VERTICAL_FOV,	vsSourceDepth.getVerticalFieldOfView() );
	vsVirDepth.setProperty( ONI_STREAM_PROPERTY_HORIZONTAL_FOV,	vsSourceDepth.getHorizontalFieldOfView() );
	vsVirDepth.setProperty( ONI_STREAM_PROPERTY_MIRRORING,		vsSourceDepth.getMirroringEnabled() );

	// assign dpeth only properties
	vsVirDepth.setProperty( ONI_STREAM_PROPERTY_MIN_VALUE,		vsSourceDepth.getMinPixelValue() );
	vsVirDepth.setProperty( ONI_STREAM_PROPERTY_MAX_VALUE,		vsSourceDepth.getMaxPixelValue() );

	// assign NiTE required properties
	mNiTEProp.WriteProperties( vsVirDepth );

	// set VideoMode
	vsVirDepth.setVideoMode( vsSourceDepth.getVideoMode() );

	// add listener for update frame
	vsSourceDepth.addNewFrameListener( new CFrameCopy( vsVirDepth ) );

	//TODO: Check if color is required

	#pragma endregion

	// create recorder
	Recorder mRecorder;
	mRecorder.create( sTarget.c_str() );
	mRecorder.attach( vsVirDepth );

	// create Playback controller
	PlaybackControl* pPlay = devSourceFile.getPlaybackControl();
	pPlay->setRepeatEnabled( false );
	pPlay->setSpeed( 0.0f );
	int iFrameNum = pPlay->getNumberOfFrames(vsSourceDepth);

	// start
	cout << "Start to re-recorder\n";
	cout << "There are " << iFrameNum << " frames in dpeth" << endl;

	mRecorder.start();
	vsSourceDepth.start();
	vsVirDepth.start();
	int i = 0;
	while( true )
	{
		pPlay->seek( vsSourceDepth, ++i );
		VideoFrameRef vfFrame;
		vsVirDepth.readFrame( &vfFrame );
		cout << "." << flush;
		if( i == iFrameNum )
			break;
	}
	cout << "Done" << endl;

	// stop
	mRecorder.stop();
	
	vsVirDepth.stop();
	vsVirDepth.destroy();
	devVirDevice.close();

	vsSourceDepth.stop();
	vsSourceDepth.destroy();
	devSourceFile.close();

	OpenNI::shutdown();
}
