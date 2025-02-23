#include "stdafx.h"
#include "Contact.h"

void Contact::CalculateInternals( float dt )
{
	if ( !_bodies[0] ) SwapBodies();
	assert( _bodies[0] );
	CalculateContactBasis();

	_relativeContactPosition[0] = _contactPoint - _bodies[0]->GetPosition();
	if ( _bodies[1] )
		_relativeContactPosition[1] = _contactPoint - _bodies[1]->GetPosition();

	_contactVelocity = CalculateLocalVelocity( 0u, dt );
	if ( _bodies[1] )
		_contactVelocity -= CalculateLocalVelocity( 1u, dt );

	CalculateDesiredDeltaVelocity( dt );
}

void Contact::SwapBodies()
{
	_contactNormal *= -1.0f;

	RigidBody* temp = _bodies[0];
	_bodies[0] = _bodies[1];
	_bodies[1] = temp;
}

void Contact::MatchAwakeState()
{
	if ( !_bodies[1] ) return;

	bool bodyA = _bodies[0]->GetActive();
	bool bodyB = _bodies[1]->GetActive();

	if ( bodyA ^ bodyB )
	{
		if ( bodyA )
			_bodies[1]->SetActive();
		else
			_bodies[0]->SetActive();
	}
}

void Contact::CalculateDesiredDeltaVelocity( float dt )
{
	static constexpr float velocityLimit = 0.25f;
	float velocityFromAcc = 0.0f;

	if ( _bodies[0]->GetActive() )
		velocityFromAcc += _bodies[0]->GetLastFrameAcceleration() * dt * _contactNormal;

	if ( _bodies[1] && _bodies[1]->GetActive() )
		velocityFromAcc -= _bodies[1]->GetLastFrameAcceleration() * dt * _contactNormal;

	float thisRestitution = _restitution;
	if ( fabsf( _contactVelocity.x ) < velocityLimit )
		thisRestitution = 0.0f;

	_desiredDeltaVelocity = -_contactVelocity.x - thisRestitution * ( _contactVelocity.x - velocityFromAcc );
}

v3df Contact::CalculateLocalVelocity( uint32_t bodyIndex, float dt )
{
	RigidBody* thisBody = _bodies[bodyIndex];
	v3df velocity = thisBody->GetRotation() % _relativeContactPosition[bodyIndex];
	velocity += thisBody->GetVelocity();

	v3df contactVelocity = _contactToWorld.TransformTranspose( velocity );
	v3df accVelocity = thisBody->GetLastFrameAcceleration() * dt;
	accVelocity = _contactToWorld.TransformTranspose( accVelocity );
	accVelocity.x = 0.0f;
	_contactVelocity += accVelocity;

	return contactVelocity;
}

void Contact::CalculateContactBasis()
{
	v3df contactTangent[2];

	if ( fabsf( _contactNormal.x ) > fabsf( _contactNormal.y ) )
	{	
		const float scaleFactor = 1.0f / sqrtf( _contactNormal.x * _contactNormal.x );

		contactTangent[0].x = _contactNormal.z * scaleFactor;
		contactTangent[0].y = 0.0f;
		contactTangent[0].z = -_contactNormal.x * scaleFactor;

		contactTangent[1].x = _contactNormal.y * contactTangent[0].x;
		contactTangent[1].y = _contactNormal.z * contactTangent[0].x - _contactNormal.x * contactTangent[0].z;
		contactTangent[1].z = -_contactNormal.y * contactTangent[0].x;
	}
	else {
		const float scaleFactor = 1.0f / sqrtf( _contactNormal.z * _contactNormal.z + _contactNormal.y * _contactNormal.y );

		contactTangent[0].x = 0.0f;
		contactTangent[0].y = -_contactNormal.z * scaleFactor;
		contactTangent[0].z = _contactNormal.y * scaleFactor;

		contactTangent[1].x = _contactNormal.y * contactTangent[0].z - _contactNormal.z * contactTangent[0].y;
		contactTangent[1].y = -_contactNormal.x * contactTangent[0].z;
		contactTangent[1].z = _contactNormal.x * contactTangent[0].y;
	}

	_contactToWorld.SetComponents( _contactNormal, contactTangent[0], contactTangent[1] );
}

void Contact::ApplyVelocityChange( v3df velocityChange[2], v3df rotationChange[2] )
{
	// get inverse mass && inverse inertia tensor from world coordinates
	Matrix3 inverseInertiaTensor[2];
	_bodies[0]->GetInverseInertiaTensorWorld( &inverseInertiaTensor[0] );
	if ( _bodies[1] )
		_bodies[1]->GetInverseInertiaTensorWorld( &inverseInertiaTensor[1] );

	// calculate impulse for contact axes
	v3df impulseContact;
	if ( _friction == 0.0f )
		impulseContact = CalculateFrictionlessImpulse( inverseInertiaTensor );
	else
		impulseContact = CalculateFrictionImpulse( inverseInertiaTensor );

	// convert impulse to world coordinates
	v3df impulse = _contactToWorld.Transform( impulseContact );

	// split in the impulse into linear and rotational components
	v3df impulsiveTorque = _relativeContactPosition[0] % impulse;
	rotationChange[0] = inverseInertiaTensor[0].Transform( impulsiveTorque );
	velocityChange[0] = { 0.0f, 0.0f, 0.0f };
	velocityChange[0].AddScaledVector( impulse, _bodies[0]->GetInverseMass() );

	_bodies[0]->AddVelocity( velocityChange[0] );
	_bodies[0]->AddRotation( rotationChange[0] );

	if ( _bodies[1] )
	{
		// calculate linear/angular changes to second rigid body
		v3df impulsiveTorque = impulse % _relativeContactPosition[1];
		rotationChange[1] = inverseInertiaTensor[1].Transform( impulsiveTorque );
		velocityChange[1] = { 0.0f, 0.0f, 0.0f };
		velocityChange[1].AddScaledVector( impulse, -_bodies[1]->GetInverseMass() );

		_bodies[1]->AddVelocity( velocityChange[1] );
		_bodies[1]->AddRotation( rotationChange[1] );
	}
}

void Contact::ApplyPositionChange( v3df linearChange[2], v3df angularChange[2], float penetration )
{
	const float angularLimit = 0.2f;
	float angularMove[2];
	float linearMove[2];

	float totalInertia = 0.0f;
	float linearInertia[2];
	float angularInertia[2];

	// calculate inertia of each object in the normal direction
	for ( uint32_t i = 0u; i < 2u; i++ )
	{
		if ( _bodies[i] )
		{
			Matrix3 inverseInertiaTensor;
			_bodies[i]->GetInverseInertiaTensorWorld( &inverseInertiaTensor );

			// calculate angular inertia the same way as frictionless velocity 
			v3df angularInertiaWorld = _relativeContactPosition[i] % _contactNormal;
			angularInertiaWorld = inverseInertiaTensor.Transform( angularInertiaWorld );
			angularInertiaWorld = angularInertiaWorld % _relativeContactPosition[i];
			angularInertia[i] = angularInertiaWorld * _contactNormal;

			linearInertia[i] = _bodies[i]->GetInverseMass();
			totalInertia += linearInertia[i] + angularInertia[i];
		}
	}

	for ( uint32_t i = 0u; i < 2u; i++ )
	{
		if ( _bodies[i] )
		{
			float sign = ( i == 0u ) ? 1.0f : -1.0f;
			angularMove[i] = sign * penetration * ( angularInertia[i] / totalInertia );
			linearMove[i] = sign * penetration * ( linearInertia[i] / totalInertia );

			v3df projection = _relativeContactPosition[i];
			projection.AddScaledVector( _contactNormal, -_relativeContactPosition[i].ScalarProduct( _contactNormal ) );
			float maxMagnitude = angularLimit * projection.magnitude();

			if ( angularMove[i] < -maxMagnitude )
			{
				float totalMove = angularMove[i] + linearMove[i];
				angularMove[i] = -maxMagnitude;
				linearMove[i] = totalMove - angularMove[i];
			}
			else if ( angularMove[i] > maxMagnitude )
			{
				float totalMove = angularMove[i] + linearMove[i];
				angularMove[i] = maxMagnitude;
				linearMove[i] = totalMove - angularMove[i];
			}

			if ( angularMove[i] == 0.0f )
				angularChange[i] = { 0.0f, 0.0f, 0.0f };
			else
			{
				v3df targetAngularDirection = _relativeContactPosition[i].VectorProduct( _contactNormal );
				Matrix3 inverseInertiaTensor;
				_bodies[i]->GetInverseInertiaTensorWorld( &inverseInertiaTensor );
				angularChange[i] = inverseInertiaTensor.Transform( targetAngularDirection ) * ( angularMove[i] / angularInertia[i] );
			}
			linearChange[i] = _contactNormal * linearMove[i];

			v3df pos = _bodies[i]->GetPosition();
			pos.AddScaledVector( _contactNormal, linearMove[i] );
			_bodies[i]->SetPosition( pos );

			Quaternion q;
			_bodies[i]->GetOrientation( &q );
			q.AddScaledVector( angularChange[i], 1.0f );
			_bodies[i]->SetOrientation( q );

			if ( !_bodies[i]->GetActive() )
				_bodies[i]->CalculateDerivedData();
		}
	}
}

v3df Contact::CalculateFrictionlessImpulse( Matrix3* inverseIntertiaTensor )
{
	v3df deltaVelWorld = _relativeContactPosition[0] % _contactNormal;
	deltaVelWorld = inverseIntertiaTensor[0].Transform( deltaVelWorld );
	deltaVelWorld = deltaVelWorld % _relativeContactPosition[0];

	float deltaVelocity = deltaVelWorld * _contactNormal;
	deltaVelocity += _bodies[0]->GetInverseMass();

	if ( _bodies[1] )
	{
		v3df deltaVelWorld = _relativeContactPosition[1] % _contactNormal;
		deltaVelWorld = inverseIntertiaTensor[1].Transform( deltaVelWorld );
		deltaVelWorld = deltaVelWorld % _relativeContactPosition[1];
		deltaVelocity += deltaVelWorld * _contactNormal;
		deltaVelocity += _bodies[1]->GetInverseMass();
	}
	
	v3df impulseContact;
	impulseContact.x = _desiredDeltaVelocity / deltaVelocity;
	impulseContact.y = 0.0f;
	impulseContact.z = 0.0f;
	return impulseContact;
}

v3df Contact::CalculateFrictionImpulse( Matrix3* inverseInertiaTensor )
{
	float inverseMass = _bodies[0]->GetInverseMass();

	// build matrix to convert between linear and angular
	Matrix3 impulseToTorque;
	impulseToTorque.SetSkewSymmetric( _relativeContactPosition[0] );

	// build matrix needed to convert contact impulse to velocity change
	Matrix3 deltaVelWorld = impulseToTorque;
	deltaVelWorld *= inverseInertiaTensor[0];
	deltaVelWorld *= impulseToTorque;
	deltaVelWorld *= -1;

	if ( _bodies[1] )
	{
		// set the cross product matrix
		impulseToTorque.SetSkewSymmetric( _relativeContactPosition[1] );

		// calculate the velocity change matrix
		Matrix3 deltaVelWorld2 = impulseToTorque;
		deltaVelWorld2 *= inverseInertiaTensor[1];
		deltaVelWorld2 *= impulseToTorque;
		deltaVelWorld2 *= -1;
		deltaVelWorld += deltaVelWorld2;
		inverseMass += _bodies[1]->GetInverseMass();
	}

	Matrix3 deltaVelocity = _contactToWorld.Transpose();
	deltaVelocity *= deltaVelWorld;
	deltaVelocity *= _contactToWorld;

	// add change in linear velocity
	deltaVelocity._data[0] += inverseMass;
	deltaVelocity._data[4] += inverseMass;
	deltaVelocity._data[8] += inverseMass;
	
	Matrix3 impulseMatrix = deltaVelocity.Inverse();
	v3df velKill( _desiredDeltaVelocity, -_contactVelocity.y, -_contactVelocity.z );
	v3df impulseContact = impulseMatrix.Transform( velKill );

	// check for excess friction
	float planarImpulse = sqrtf(
		impulseContact.y * impulseContact.y +
		impulseContact.z * impulseContact.z
	);

	if ( planarImpulse > impulseContact.x * _friction )
	{
		// use dynamic friction
		impulseContact.y /= planarImpulse;
		impulseContact.z /= planarImpulse;

		impulseContact.x =
			deltaVelocity._data[0] +
			deltaVelocity._data[1] * _friction * impulseContact.y +
			deltaVelocity._data[2] * _friction * impulseContact.z;
		impulseContact.x = _desiredDeltaVelocity / impulseContact.x;
		impulseContact.y *= _friction * impulseContact.x;
		impulseContact.z *= _friction * impulseContact.x;
	}
	return impulseContact;
}