#include <iostream>
//#include "PlayerChar.h"
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>
#include <assert.h>
#include <fstream>
#include <list> 
#include <stdlib.h>
#include "EditSession.h"
#include "VectorMath.h"

#define TIMESTEP 1.f / 60.f

using namespace std;
using namespace sf;

RenderWindow *window;



struct Edge
{
	Vector2f v0;
	Vector2f v1;
	Edge * GetEdge0();
	Edge * GetEdge1();
	Edge *edge0;
	Edge *edge1;

	//material ---
	Edge()
	{
		
	}

	Vector2f Normal()
	{
		Vector2f v = v1 - v0;
		Vector2f temp = normalize( v );
		return Vector2f( temp.y, -temp.x );
	}

	Vector2f GetPoint( float quantity )
	{
		//gets the point on a line w/ length quantity in the direction of the edge vector
		Vector2f e( v1 - v0 );
		e = normalize( e );
		return v0 + quantity * e;
	}

	

	float GetQuantity( Vector2f p )
	{
		//projects the origin of the line to p onto the edge. if the point is on the edge it will just be 
		//normal to use dot product to get cos(0) =1
		Vector2f vv = normalize(p - v0);
		Vector2f e = normalize(v1 - v0);
		return dot( vv, e ) * length( p - v0 );                          
	}
};



struct CollisionBox
{

	enum BoxType
	{
		Physics,
		Hit,
		Hurt
	};

	Vector2f offset;
	float offsetAngle;
	
	float rw; //radius or half width
	float rh; //radius or half height
	bool isCircle;
	BoxType type;

	Vector2f GetAxis1( Vector2f actorPos )
	{
	/*	float left = actorPos.x + offset.x - rw;
		float right = actorPos.x + offset.x + rw;
		float top = actorPos.y + offset.y - rh;
		float bottom = actorPos.y + offset.y + rh;
		Vector2f topLeft( left, top );
		Vector2f topRight( right, top );
		Vector2f bottomLeft( left, bottom );

		topLeft.x = cos( offsetAngle ) * topLeft.x + sin( offsetAngle ) * topLeft.y;
		topLeft.y = -sin( offsetAngle ) * topLeft.x + cos( offsetAngle ) * topLeft.y;*/
	}
};

struct Contact
{
	Contact()
		:edge( NULL )
	{
		collisionPriority = 0;
	}
		
	float collisionPriority;	
	Vector2f position;
	Vector2f resolution;
	Edge *edge;
};



struct Tileset
{
	IntRect GetSubRect( int localID )
	{
		int xi,yi;
		yi = localID / (texture->getSize().x / tileWidth );
		xi = localID % (texture->getSize().x / tileWidth );

		return IntRect( xi * tileWidth, yi * tileHeight, tileWidth, tileHeight ); 
	}

	Texture * texture;
	int tileWidth;
	int tileHeight;
	string sourceName;
};

list<Tileset*> tilesetList;

Tileset * GetTileset( const string & s, int tileWidth, int tileHeight )
{
	for( list<Tileset*>::iterator it = tilesetList.begin(); it != tilesetList.end(); ++it )
	{
		if( (*it)->sourceName == s )
		{
			return (*it);
		}
	}


	//not found


	Tileset *t = new Tileset();
	t->texture = new Texture();
	t->texture->loadFromFile( s );
	t->tileWidth = tileWidth;
	t->tileHeight = tileHeight;
	tilesetList.push_back( t );

	return t;
	//make sure to set up tileset here
}

struct Actor
{
	enum Action
	{
		STAND,
		RUN,
		JUMP,
		Count
	};

	Sprite *sprite;
	Tileset *tilesetStand;
	Tileset *tilesetRun;

	int actionLength[Action::Count]; //actionLength-1 is the max frame counter for each action
	Actor::Actor()
	{
		sprite = new Sprite;
		velocity = Vector2f( 0, 0 );
		actionLength[STAND] = 18 * 4;
		tilesetStand = GetTileset( "stand.png", 64, 64 );

		actionLength[RUN] = 10 * 4;
		tilesetRun = GetTileset( "run.png", 128, 64 );

		actionLength[JUMP] = 5;

		currentAction = RUN;
		frame = 0;
	}

	void ActionEnded()
	{
		if( frame >= actionLength[currentAction] )
		{
			switch( currentAction )
			{
			case STAND:
				frame = 0;
				break;
			case RUN:
				frame = 0;
				break;
			}
		}
	}

	void UpdatePrePhysics()
	{
		ActionEnded();


		switch( currentAction )
		{
		case STAND:
			break;
		case RUN:
			break;
		case JUMP:
			break;
		}

	//	cout << "position: " << position.x << ", " << position.y << endl;
	//	cout << "velocity: " << velocity.x << ", " << velocity.y << endl;
		
	}

	void UpdatePhysics()
	{
	}

	void UpdatePostPhysics()
	{
		//cout << "updating" << endl;
		switch( currentAction )
		{
		case STAND:
				//display stand at the current frame
			sprite->setPosition( position );
			
			sprite->setTexture( *(tilesetStand->texture));
			sprite->setTextureRect( tilesetStand->GetSubRect( frame / 4 ) );
			sprite->setOrigin( sprite->getLocalBounds().width / 2, sprite->getLocalBounds().height / 2 );
			//cout << "setting to frame: " << frame / 4 << endl;
			break;
		case RUN:
			sprite->setPosition( position );
			
			sprite->setTexture( *(tilesetRun->texture));
			sprite->setTextureRect( tilesetRun->GetSubRect( frame / 4 ) );
			sprite->setOrigin( sprite->getLocalBounds().width / 2, sprite->getLocalBounds().height / 2 );
			break;
		case JUMP:
			break;
		}

		++frame;
	}


	Action currentAction;
	int frame;
	Vector2f position;
	Vector2f velocity;
	CollisionBox *physBox;
};

struct LineIntersection
{
	LineIntersection(const Vector2f &pos, bool p )
	{
		position = pos;
		parallel = p;
	}

	Vector2f position;
	bool parallel;
};

LineIntersection lineIntersection( Vector2f a, Vector2f b, Vector2f c, Vector2f d )
{
	double ax = a.x;
	double ay = a.y;
	double bx = b.x;
	double by = b.y;
	double cx = c.x;
	double cy = c.y;
	double dx = d.x;
	double dy = d.y;

	double x= 0,y = 0;
	bool parallel = false;
	if( (ax-bx)*(cy - dy) - (ay - by) * (cx - dx ) == 0 )
	{
		parallel = true;
	}
	else
	{
		x = ((ax * by - ay * bx ) * ( cx - dx ) - (ax - bx ) * ( cx * dy - cy * dx ))
			/ ( (ax-bx)*(cy - dy) - (ay - by) * (cx - dx ) );
		y = ((ax * by - ay * bx ) * ( cy - dy ) - (ay - by ) * ( cx * dy - cy * dx ))
			/ ( (ax-bx)*(cy - dy) - (ay - by) * (cx - dx ) );
	}

	return LineIntersection( Vector2f(x,y), parallel );
}

Contact *currentContact;
Contact *collideEdge( Actor &a, const CollisionBox &b, Edge *e )
{
	Vector2f oldPosition = a.position - a.velocity;
	float left = a.position.x + b.offset.x - b.rw;
	float right = a.position.x + b.offset.x + b.rw;
	float top = a.position.y + b.offset.y - b.rh;
	float bottom = a.position.y + b.offset.y + b.rh;

	

	float oldLeft = oldPosition.x + b.offset.x - b.rw;
	float oldRight = oldPosition.x + b.offset.x + b.rw;
	float oldTop = oldPosition.y + b.offset.y - b.rh;
	float oldBottom = oldPosition.y + b.offset.y + b.rh;


	float edgeLeft = min( e->v0.x, e->v1.x );
	float edgeRight = max( e->v0.x, e->v1.x ); 
	float edgeTop = min( e->v0.y, e->v1.y ); 
	float edgeBottom = max( e->v0.y, e->v1.y ); 

	bool aabbCollision = false;
	if( left <= edgeRight && right >= edgeLeft && top <= edgeBottom && bottom >= edgeTop )
	{	
		
		aabbCollision = true;
		
	}

	Vector2f half = (oldPosition + a.position ) / 2.f;
		float halfLeft = half.x - b.rw;
		float halfRight = half.x + b.rw;
		float halfTop = half.y - b.rh;
		float halfBottom = half.y + b.rh;

	if( halfLeft <= edgeRight && halfRight >= edgeLeft && halfTop <= edgeBottom && halfBottom >= edgeTop )
		{
			aabbCollision = true;
		}

	if( aabbCollision )
	{
		Vector2f corner(0,0);
		Vector2f edgeNormal = e->Normal();

		if( edgeNormal.x > 0 )
		{
			corner.x = left;
		}
		else if( edgeNormal.x < 0 )
		{
			corner.x = right;
		}
		else
		{
			if( edgeLeft <= left )
				corner.x = left;
			else if ( edgeRight >= right )
				corner.x = right;
			else
			{
				corner.x = (edgeLeft + edgeRight) / 2;
			}
			//aabb
			//cout << "this prob" << endl;
		}

		if( edgeNormal.y > 0 )
		{
			corner.y = top;
		}
		else if( edgeNormal.y < 0 )
		{
			corner.y = bottom;
		}
		else
		{
			//aabb
			if( edgeTop <= top )
			corner.y = top;
			else if ( edgeBottom >= bottom )
				corner.y = bottom;
			else
				corner.y = (edgeTop+ edgeBottom) / 2;
			//else
			//cout << "this 2" << endl;
		}

		int res = cross( corner - e->v0, e->v1 - e->v0 );

		double measureNormal = dot( edgeNormal, normalize(-a.velocity) );

		if( res < 0 && measureNormal >= 0 && ( a.velocity.x != 0 || a.velocity.y != 0 )  )	
		{
			
			Vector2f invVel = normalize(-a.velocity);



			LineIntersection li = lineIntersection( corner, corner - (a.velocity), e->v0, e->v1 );

			//assert( li.parallel == false );
			if( li.parallel )
			{
				return NULL;
			}
			Vector2f intersect = li.position;

			float intersectQuantity = e->GetQuantity( intersect );
			Vector2f collisionPosition = intersect;
			if( intersectQuantity < 0 )
			{
				collisionPosition = e->v0;
				cout << "under: " << e->v0.x << ", " << e->v0.y << endl;
				float minx = min( intersect.x, corner.x );
				float maxx = max( intersect.x, corner.x );
				float miny = min( intersect.y, corner.y );
				float maxy = max( intersect.y, corner.y );

				LineIntersection vertical = lineIntersection( intersect, corner, e->v0, Vector2f( e->v0.x, e->v0.y - 1 ) );
			//	intersect = lii.position;

				LineIntersection horizontal = lineIntersection( intersect, corner, e->v0, Vector2f( e->v0.x-1, e->v0.y ) );
			//	intersect = lii1.position;

				float verticalDist = dot( vertical.position - corner, intersect - corner );//length( lii.position - corner )* dot(normalize(lii.position - corner), normalize( intersect - corner )) ;
				float horizontalDist =dot( horizontal.position - corner, intersect - corner ); //length( lii1.position - corner )* dot(normalize(lii1.position - corner), normalize( intersect - corner )) ;

//				if( distanceI <= 0 && distanceI1 <= 0 )
//					assert( false && "bbbbsdfdf" );
				bool vertOK = false;
				bool horiOK = false;
				if( !vertical.parallel && verticalDist >= 0 )// && length(vertical.position - corner) < length( intersect - corner ) )
				{
					vertOK = true;
				}

				if( !horizontal.parallel && horizontalDist >= 0 )//&& length(horizontal.position - corner) < length( intersect - corner ))
				{
					horiOK = true;
				}
				if( vertOK && horiOK ) 
				{
					if( verticalDist <= horizontalDist)//length( vertical.position - corner ) < length( horizontal.position - corner ) )
					{
						intersect = vertical.position;
					}
					else
					{
						intersect = horizontal.position;
					}
					//intersect = lii.position;
				//	cout << "case 0" << endl;
				}
				else if( vertOK )
				{
					intersect = vertical.position;
				//	cout << "case 1" << endl;
				}
				else if( horiOK )
				{
					//cout << "case 2" << endl;
					intersect = horizontal.position;
				}
				else
				{
					cout << "case failure" << endl;
					//return NULL;
				//	assert( false && "case error" );
				}
			}
			else if( intersectQuantity > length( e->v1 - e->v0 ) )
			{
				collisionPosition = e->v1;
				cout << "over: " << e->v0.x << ", " << e->v0.y << endl;
				float minx = min( intersect.x, corner.x );
				float maxx = max( intersect.x, corner.x );
				float miny = min( intersect.y, corner.y );
				float maxy = max( intersect.y, corner.y );

				LineIntersection vertical = lineIntersection( intersect, corner, e->v1, Vector2f( e->v1.x, e->v1.y - 1 ) );
			//	intersect = lii.position;

				LineIntersection horizontal = lineIntersection( intersect, corner, e->v1, Vector2f( e->v1.x-1, e->v1.y ) );
			//	intersect = lii1.position;

				float verticalDist = dot( vertical.position - corner, intersect - corner );//length(lii.position - corner) * dot(normalize(lii.position - corner), normalize( intersect - corner )) ;
				float horizontalDist = dot( horizontal.position - corner, intersect - corner );//length( lii1.position - corner ) * dot(normalize(lii1.position - corner), normalize( intersect - corner )) ;

			//	if( distanceI < 0 && distanceI1 < 0 )
				//	assert( false && "bbbbsdfdf" );
				bool vertOK = false;
				bool horiOK = false;
				if( !vertical.parallel && verticalDist >= 0 )// && length(vertical.position - corner) < length( intersect - corner ) )
				{
					vertOK = true;
				}

				if( !horizontal.parallel && horizontalDist >= 0 )//&& length(horizontal.position - corner) < length( intersect - corner ))
				{
					horiOK = true;
				}
				if( vertOK && horiOK ) 
				{
					if( verticalDist <= horizontalDist )//length( vertical.position - corner ) < length( horizontal.position - corner ) )
					{
						intersect = vertical.position;
					}
					else
					{
						intersect = horizontal.position;
					}
					//intersect = lii.position;
				//	cout << "case 0" << endl;
				}
				else if( vertOK )
				{
					intersect = vertical.position;
				//	cout << "case 1" << endl;
				}
				else if( horiOK )
				{
					//cout << "case 2" << endl;
					intersect = horizontal.position;
				}
				else
				{
					cout << "case failure" << endl;
					//return NULL;
				//	assert( false && "case error" );
				}

			
			}
			if( dot( normalize( -a.velocity ), normalize(intersect - corner ) ) < .999 )
			{
			//	return NULL;
			}
			double pri = dot( intersect - ( corner - a.velocity ), normalize( a.velocity ) );
				//cross( (corner - a.velocity) - e->v0, normalize( e->v1 - e->v0 ) );
			//double pri = -cross( normalize((corner) - e->v0), normalize( e->v1 - e->v0 ) );
			//double pri = length( intersect - (corner - a.velocity ) );
			//cout << "pri: " << pri <<" .... " << e->v0.x << ", " << e->v0.y << " .. " << e->v1.x << ", " << e->v1.y << endl;
			if( pri < -1 )
			{
				cout << "BUSTED--------------- " << pri  << endl;
				return NULL;
			}

			intersectQuantity = e->GetQuantity( intersect );
			//cout << "intersectQuantity2222: " << intersectQuantity << endl;
			

			//cs.setPosition( bottomLeft - invVel );
			//window->draw( cs );

			//Vector2f p = intersect - corner;
			currentContact->position = collisionPosition;//intersect;
			currentContact->resolution = intersect - corner;
			currentContact->collisionPriority = pri;//dot(intersect - ( corner + invVel), e->Normal());
			currentContact->edge = e;

			return currentContact;
			//a.position = a.position + p;
		}

		
	}

	return NULL;
}

Contact *collideWithEdges( Actor &a, const CollisionBox &b )
{
}

void collideShapes( Actor &a, const CollisionBox &b, Actor &a1, const CollisionBox &b1 )
{
	if( b.isCircle && b1.isCircle )
	{
		//circle circle
	}
	else if( b.isCircle )
	{
		//circle rect
	}
	else if( b1.isCircle )
	{
		//circle rect
	}
	else
	{
		//rect rect
	}
}

int main()
{
	window = new sf::RenderWindow(/*sf::VideoMode(1400, 900)sf::VideoMode::getDesktopMode()*/
		sf::VideoMode( 1920 / 1, 1080 / 1), "Breakneck", sf::Style::Default, sf::ContextSettings( 0, 0, 0, 0, 0 ));
	//window->setPosition( Vector2i(800, 0 ));
	sf::Vector2i pos( 0, 0 );

	//window->setPosition( pos );
	window->setVerticalSyncEnabled( true );
	//window->setFramerateLimit( 60 );
	window->setMouseCursorVisible( true );
	
	View view( sf::Vector2f(  300, 300 ), sf::Vector2f( 960, 540 ) );
	window->setView( view );

	EditSession es(window );
	es.Run( "test1" );

	window->setVerticalSyncEnabled( true );

	currentContact = new Contact;

	
	sf::RectangleShape bDraw;
	bDraw.setFillColor( Color::Red );
	bDraw.setSize( sf::Vector2f(32 * 2, 32 * 2) );
	bDraw.setOrigin( bDraw.getLocalBounds().width /2, bDraw.getLocalBounds().height / 2 );

	ifstream is;
	is.open( "test1.brknk" );

	int numPoints;
	is >> numPoints;
	Vector2f *points = new Vector2f[numPoints];
	list<Vector2f> lvec;

	Actor player;
	//player.position = Vector2f( -500, 300 );

	is >> player.position.x;
	is >> player.position.y;

	int pointsLeft = numPoints;

	int pointCounter = 0;

	Edge **edges = new Edge*[numPoints];
	while( pointCounter < numPoints )
	{
		string matStr;
		is >> matStr;
		int polyPoints;
		is >> polyPoints;

		int currentEdgeIndex = pointCounter;
		for( int i = 0; i < polyPoints; ++i )
		{
			float px, py;
			is >> px;
			is >> py;
			
			points[pointCounter].x = px;
			points[pointCounter].y = py;
			++pointCounter;
		}

		for( int i = 0; i < polyPoints; ++i )
		{
			Edge *ee = new Edge();
			edges[currentEdgeIndex + i] = ee;
			ee->v0 = points[i+currentEdgeIndex];
			if( i < polyPoints - 1 )
				ee->v1 = points[i+1 + currentEdgeIndex];

		

			if( i == 0 )
			{
				ee->edge0 = edges[currentEdgeIndex + polyPoints - 1];
				ee->edge1 = edges[currentEdgeIndex + 1];
			}
			else if( i == polyPoints - 1 )
			{
				ee->edge0 = edges[currentEdgeIndex + i - 1];
				ee->edge1 = edges[currentEdgeIndex];
				ee->v1 = points[currentEdgeIndex];
			}
			else
			{
				ee->edge0 = edges[currentEdgeIndex + i - 1];
				ee->edge1 = edges[currentEdgeIndex + i + 1];
			}
		}
	}
	
	sf::Vertex *line = new sf::Vertex[numPoints*2];
	for( int i = 0; i < numPoints; ++i )
	{
		//cout << "i: " << i << endl;
		line[i*2] = sf::Vertex( edges[i]->v0 );
		line[i*2+1] =  sf::Vertex( edges[i]->v1 );
	}	

	sf::Vector2f nLine = line[1].position - line[0].position;
	nLine = normalize( nLine );

	sf::Vector2f lineNormal( -nLine.y, nLine.x );

	sf::CircleShape circle( 30 );
	circle.setFillColor( Color::Blue );


	CollisionBox b;
	b.isCircle = false;
	b.offsetAngle = 0;
	b.offset.x = 0;
	b.offset.y = 0;
	b.rw = 32;
	b.rh = 32;
	b.type = b.Physics;

	

	sf::Clock gameClock;
	double currentTime = 0;
	double accumulator = TIMESTEP + .1;

	Vector2f otherPlayerPos;
	

	

	while( true )
	{
		double newTime = gameClock.getElapsedTime().asSeconds();
		double frameTime = newTime - currentTime;

		if ( frameTime > 0.25 )
			frameTime = 0.25;	
        currentTime = newTime;

		accumulator += frameTime;

		
		while ( accumulator >= TIMESTEP  )
        {
			window->clear();
			float f = 50;			
			player.velocity = Vector2f( 0, 0 );
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::Right ) )
			{
				player.velocity += Vector2f( f, 0 );
				//break;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::Left ) )
			{
				player.velocity += Vector2f( -f, 0 );
				//break;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::Up ) )
			{
				player.velocity += Vector2f( 0, -f );
				//break;
			}
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::Down ) )
			{
				player.velocity += Vector2f( 0, f );
				//break;
			}


			player.UpdatePrePhysics();

			//Vector2f rCenter( r.getPosition().x + r.getLocalBounds().width / 2, r.getPosition().y + r.getLocalBounds().height / 2 );
			float xkl = 2;
			for( int jkl = 0; jkl < xkl; ++jkl )
			{
			player.position += player.velocity / xkl;
			float minPriority = 1000000;
			Edge *minEdge = NULL;
			Vector2f res(0,0);
			int collisionNumber = 0;
			for( int i = 0; i < numPoints; ++i )
			{
				Contact *c = collideEdge( player, b, edges[i] );
				if( c != NULL )
				{
					collisionNumber++;
					if( c->collisionPriority <= minPriority )
					{
						if( c->collisionPriority == minPriority )
						{
							if( length(c->resolution) > length(res) )
							{
								minPriority = c->collisionPriority;
								minEdge = edges[i];
								res = c->resolution;
							}
						}
						else
						{

							minPriority = c->collisionPriority;
							minEdge = edges[i];
							res = c->resolution;
						}
					}
				}
			}


			cout << "collisionNumber: " << collisionNumber << endl;
			if( minEdge != NULL )
			{
				
				Contact *c = collideEdge( player, b, minEdge );
				//cout << "priority at: " << minEdge->Normal().x << ", " << minEdge->Normal().y << ": " << minPriority << endl;
				otherPlayerPos = player.position;
				player.position += c->resolution;

				sf::CircleShape cs;
				cs.setFillColor( Color::Magenta );
				
				cs.setRadius( 20 );
				cs.setOrigin( cs.getLocalBounds().width / 2, cs.getLocalBounds().height / 2 );
				cs.setPosition( c->position );
				window->draw( cs );
				cout << "resolution: " << c->resolution.x << ", " << c->resolution.y << endl;
				//cout << "cpos: " << c->position.x << ", " << c->position.y << endl;
				Color lc = Color::Green;
				sf::Vertex linez[] =
				{
					sf::Vertex(sf::Vector2f(minEdge->v0.x + 1, minEdge->v0.y + 1), lc),
					sf::Vertex(sf::Vector2f(minEdge->v1.x + 1, minEdge->v1.y + 1), lc),
					sf::Vertex(sf::Vector2f(minEdge->v0.x - 1, minEdge->v0.y - 1), lc),
					sf::Vertex(sf::Vector2f(minEdge->v1.x - 1, minEdge->v1.y - 1), lc)
				};

				window->draw(linez, 4, sf::Lines);
				 
			}
			}
			

			player.UpdatePostPhysics();

			//r.setPosition( player.position.x - r.getLocalBounds().width / 2, player.position.y - r.getLocalBounds().height / 2 );
			//cout << "playerPos: " << player.position.x << ", " << player.position.y << endl;	
		

			accumulator -= TIMESTEP;
			while( sf::Keyboard::isKeyPressed( sf::Keyboard::Space ) )
		{
		}
		}

		view.setSize( Vector2f( 960 * 1, 540 * 1 ) );
		view.setCenter( player.position );
		window->setView( view );

		bDraw.setPosition( player.position );
		window->draw( bDraw );

		window->draw( *(player.sprite) );
		sf::RectangleShape rs;
		rs.setSize( Vector2f(64, 64) );
		rs.setOrigin( rs.getLocalBounds().width / 2, rs.getLocalBounds().height / 2 );
		rs.setPosition( otherPlayerPos );
		rs.setFillColor( Color::Blue );
		window->draw( circle );
		window->draw(line, numPoints * 2, sf::Lines);

		window->display();

	/*	while( true )
		{
			if( !sf::Keyboard::isKeyPressed( sf::Keyboard::Right ) )
				break;
			if( !sf::Keyboard::isKeyPressed( sf::Keyboard::Left ) )
				break;
			if( !sf::Keyboard::isKeyPressed( sf::Keyboard::Up ) )
				break;
			if( !sf::Keyboard::isKeyPressed( sf::Keyboard::Down) )
				break;
		}*/

		if( sf::Keyboard::isKeyPressed( sf::Keyboard::Escape ) )
				return 0;

		


		
		
		
	}

	window->close();
	delete window;
}

