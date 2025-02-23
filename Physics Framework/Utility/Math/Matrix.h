#pragma once
#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

template<typename T>
class Matrix
{
public:
	Matrix( uint32_t _rows, uint32_t _cols, const T& _initial )
	{
		mat.resize( _rows );
		for ( uint32_t i = 0; i < mat.size(); i++ )
			mat[i].resize( _cols, _initial );
		rows = _rows;
		cols = _cols;
	}
	Matrix( const Matrix<T>& rhs ) { mat = rhs.mat; rows = rhs.get_rows(); cols = rhs.get_cols(); }
	virtual ~Matrix() {}

	// Operator overloading, for "standard" mathematical matrix operations
	Matrix<T>& operator = ( const Matrix<T>& rhs )
	{
		if ( &rhs == this ) return *this;
		uint32_t new_rows = rhs.get_rows();
		uint32_t new_cols = rhs.get_cols();

		rows = new_rows;
		cols = new_cols;
		
		mat.resize( new_rows );
		for ( uint32_t i = 0; i < mat.size(); i++ )
			mat[i].resize( new_cols );

		for ( uint32_t i = 0; i < new_rows; i++ )
			for ( uint32_t j = 0; j < new_cols; j++ )
				mat[i][j] = rhs( i, j );

		return *this;
	}

	// Matrix mathematical operations
	Matrix<T> operator + ( const Matrix<T>& rhs )
	{
		Matrix result( rows, cols, 0.0 );

		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] + rhs( i, j );

		return result;
	}
	Matrix<T> operator - ( const Matrix<T>& rhs )
	{
		uint32_t rows = rhs.get_rows();
		uint32_t cols = rhs.get_cols();
		Matrix result( rows, cols, 0.0 );

		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] - result( i. j );

		return result;
	}
	Matrix<T> operator * ( const Matrix<T>& rhs )
	{
		uint32_t rows = rhs.get_rows();
		uint32_t cols = rhs.get_cols();
		Matrix result( rows, cols, 0.0 );

		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				for ( uint32_t k = 0; k < rows; k++ )
					result( i, j ) = mat[i][k] * result( k. j );

		return result;
	}
	Matrix<T>& operator += ( const Matrix<T>& rhs )
	{
		uint32_t rows = rhs.get_rows();
		uint32_t cols = rhs.get_cols();

		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				mat[i][j] += rhs( i, j );

		return *this;
	}
	Matrix<T>& operator -= ( const Matrix<T>& rhs )
	{
		uint32_t rows = rhs.get_rows();
		uint32_t cols = rhs.get_cols();

		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				mat[i][j] -= rhs( i, j );

		return *this;
	}
	Matrix<T>& operator *= ( const Matrix<T>& rhs )
	{
		Matrix result = (*this) * rhs;
		(*this) = result;
		return *this;
	}
	Matrix<T> transpose()
	{
		Matrix result( rows, cols, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[j][i];
		return result;
	}

	// Matrix/scalar operations
	Matrix<T> operator + ( const T& rhs )
	{
		Matrix result( rows, cols, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] + rhs;
		return result;
	}
	Matrix<T> operator - ( const T& rhs )
	{
		Matrix result( rows, cols, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] - rhs;
		return result;
	}
	Matrix<T> operator * ( const T& rhs )
	{
		Matrix result( rows, cols, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] * rhs;
		return result;
	}
	Matrix<T> operator / ( const T& rhs )
	{
		Matrix result( rows, cols, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result( i, j ) = mat[i][j] / rhs;
		return result;
	}

	// Matrix/vector operations
	std::vector<T> operator * ( const std::vector<T>& rhs )
	{
		std::vector<T> result( rhs.size(), 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			for ( uint32_t j = 0; j < cols; j++ )
				result[i] = mat[i][j] * rhs[j];
		return result;
	}
	std::vector<T> diag_vec()
	{
		std::vector<T> result( rows, 0.0 );
		for ( uint32_t i = 0; i < rows; i++ )
			result[i] = mat[i][i];
		return result;
	}

	// Access the individual elements
	T& operator () ( const uint32_t& row, const uint32_t& col ) { return mat[row][col]; }
	const T& operator () ( const uint32_t& row, const uint32_t& col ) const { return mat[row][col]; }

	// Access the row and column sizes
	uint32_t get_rows() const { return rows; }
	uint32_t get_cols() const { return cols; }

private:
	uint32_t rows;
	uint32_t cols;
	std::vector<std::vector<T>> mat;
};

typedef Matrix<float> matf;
typedef Matrix<int> mati;

#endif