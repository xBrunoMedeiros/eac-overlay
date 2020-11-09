#pragma once

/****************************************************************************************
* Author:	Gilles Bellot
* Date:		01/07/2017 - Dortmund - Germany
*
* Desc:		functional error and exception handling
*			based on the talk: C++ and Beyond 2012: Andrei Alexandrescu - Systematic Error Handling in C++
*
****************************************************************************************/

// exception handling
#include <exception>
#include <stdexcept>

// atomic
#include <atomic>

namespace util
{
	template<class T>
	class Expected
	{
	protected:
		union
		{
			T result;
			std::exception_ptr spam;
		};

		bool gotResult;							// true iff valid result is available
		Expected( ) {};							// private constructor

	public:
		// constructors and destructor
		Expected( const T& r ) : result( r ), gotResult( true ) {}
		Expected( T&& r ) : result( std::move( r ) ), gotResult( true ) {}
		Expected( const Expected& e ) : gotResult( e.gotResult )
		{
			if ( gotResult )
				new( &result ) T( e.result );
			else
				new( &spam ) std::exception_ptr( e.spam );
		}
		Expected( Expected&& e ) : gotResult( e.gotResult )
		{
			if ( gotResult )
				new( &result ) T( std::move( e.result ) );
			else
				new( &spam ) std::exception_ptr( std::move( e.spam ) );
		}
		~Expected( ) {}

		// swap two "expected"
		void swap( Expected& e )
		{
			if ( gotResult )
			{
				if ( e.gotResult )
					std::swap( result, e.result );
				else
				{
					auto t = std::move( e.spam );
					new( &e.result ) T( std::move( result ) );
					new( &spam ) std::exception_ptr;
					std::swap( gotResult, e.gotResult );
				}
			}
			else
			{
				if ( e.gotResult )
					e.swap( *this );
				else
					spam.swap( e.spam );
			}
		}

		// creating expect from exceptions
		template<typename E>
		Expected<T>( E const& e ) : spam( std::make_exception_ptr( e ) ), gotResult( false ) { }

		template<class E>
		static Expected<T> fromException( const E& exception )
		{
			if ( typeid( exception ) != typeid( E ) )
				throw std::invalid_argument( "slicing detected!\n" );
			return fromException( std::make_exception_ptr( exception ) );
		}
		static Expected<T> fromException( std::exception_ptr p )
		{
			Expected<T> e;
			e.gotResult = false;
			new( &e.spam ) std::exception_ptr( std::move( p ) );
			return e;
		}
		static Expected<T> fromException( )
		{
			return fromException( std::current_exception( ) );
		}

		// operator overload
		Expected<T>& operator=( const Expected<T>& e )
		{
			gotResult = e.gotResult;
			if ( gotResult )
				new( &result ) T( e.result );
			else
				new( &spam ) std::exception_ptr( e.spam );
			return *this;
		}

		// getters
		bool isValid( ) const { return gotResult; };
		bool wasSuccessful( ) const { return gotResult; };

		T& get( )
		{
			if ( !gotResult )
				std::rethrow_exception( spam );
			return result;
		}
		const T& get( ) const
		{
			if ( !gotResult )
				std::rethrow_exception( spam );
			return result;
		}

		// probe for exception
		template<class E>
		bool has_exception( ) const
		{
			try
			{
				if ( !gotResult )
					std::rethrow_exception( spam );
			}
			catch ( const E& object )
			{
				( void ) object;
				return true;
			}
			catch ( ... )
			{

			}
			return false;
		}

		friend class Expected<void>;
	};

	template<>
	class Expected<void>
	{
		std::exception_ptr spam;

	public:
		// constructors and destructor
		template <typename E>
		Expected( E const& e ) : spam( std::make_exception_ptr( e ) )
		{
		
		}
		
		template<typename T>
		Expected( const Expected<T>& e )
		{
			if ( !e.gotResult )
			{
				new( &spam ) std::exception_ptr( e.spam );
			}
		}

		Expected( Expected&& o ) : spam( std::move( o.spam ) )
		{
		
		}
		Expected( ) : spam( )
		{
		
		}

		// operator overload
		Expected& operator=( const Expected& e )
		{
			if ( !e.is_valid( ) )
			{
				this->spam = e.spam;
			}
			return *this;
		};

		// getters
		bool is_valid( ) const { return !spam; }
		bool was_successful( ) const { return !spam; }
		void get( ) const { if ( !is_valid( ) ) std::rethrow_exception( spam ); }
		void suppress( ) {}
	};
}