/* ----------------------------- MNI Header -----------------------------------
@NAME       : objectives.c
@DESCRIPTION: File containing objective function routines.
@METHOD     : 
@GLOBALS    : 
@COPYRIGHT  :
              Copyright 1993 Louis Collins, McConnell Brain Imaging Centre, 
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.

@CREATED    : Wed Jun  9 12:56:08 EST 1993 LC
@MODIFIED   :  $Log: objectives.c,v $
@MODIFIED   :  Revision 96.1  1997-11-03 15:06:29  louis
@MODIFIED   :  working version, before creation of mni_animal package, and before inserting
@MODIFIED   :  distance transforms
@MODIFIED   :
 * Revision 96.0  1996/08/21  18:22:10  louis
 * Release of MNI_AutoReg version 0.96
 *
 * Revision 9.6  1996/08/21  18:22:04  louis
 * Pre-release
 *
 * Revision 9.5  1996/08/12  14:16:03  louis
 * Never released version 0.95
 *
 * Revision 1.12  1996/08/12  14:15:56  louis
 * Pre-release
 *
 * Revision 1.11  1996/03/25  10:33:15  collins
 * removed the mutual info objective function and moved it
 * into the separate file obj_fn_mutual_info.c
 *
 * Revision 1.10  1996/03/07  13:25:19  collins
 * small reorganisation of procedures and working version of non-isotropic
 * smoothing.
 *
 * Revision 1.9  1995/09/07  10:05:11  collins
 * All references to numerical recipes routines are being removed.  At this
 * stage, any num rec routine should be local in the file.  All memory
 * allocation calls to vector(), matrix(), free_vector() etc... have been
 * replaced with ALLOC and FREE from the volume_io library of routines.
 *
 * Revision 1.8  1995/02/22  08:56:06  collins
 * Montreal Neurological Institute version.
 * compiled and working on SGI.  this is before any changes for SPARC/
 * Solaris.
 *
 * Revision 1.7  94/04/06  11:48:44  louis
 * working linted version of linear + non-linear registration based on Lvv
 * operator working in 3D
 * 
 * Revision 1.6  94/02/21  16:35:56  louis
 * version before feb 22 changes
 * 
 * Revision 1.5  93/11/15  16:27:07  louis
 * working version, with new library, with RCS revision stuff,
 * before deformations included
 * 
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/registration/mni_autoreg/minctracc/Optimize/objectives.c,v 96.1 1997-11-03 15:06:29 louis Exp $";
#endif

#include <internal_volume_io.h>
#include "constants.h"
#include "arg_data.h"

#include "segment_table.h"
#include "local_macros.h"
#include <print_error.h>
#include "vox_space.h"

extern Arg_Data main_args;

extern Segment_Table *segment_table;

public int point_not_masked(Volume volume, 
			    Real wx, Real wy, Real wz);

public int voxel_point_not_masked(Volume volume, 
                                  Real vx, Real vy, Real vz);



/* ----------------------------- MNI Header -----------------------------------
@NAME       : xcorr_objective
@INPUT      : volumetric data, for use in correlation.  

	      Correlation is accomplished under the currently defined
	      affine transformation in `globals->trans_info.transformation.trans_data'

	      The cross correlation is calculated using sub-sampling
	      in each of the volumes.

@OUTPUT     : none

@RETURNS    : the normalized cross correlation value (min=0.0 max = 1.0)
              0.0 means perfect fit!

@DESCRIPTION: this routine does volumetric subsampling in world space.
              These world coordinates are mapped back into each 
	      data volume to get the actual value at the given location.
	      The globals lattice info is used to calculate
              positions of the sub-samples in each vol.

	      The correlation is equal to:

	      r =    f1 / ( sqrt(f2) * sqrt(f3) )
              
	      where:          
	             f1 = sum( d1 .* d2)    (point to point multiply)
		     f2 = sum( d1 .^ 2 )    (sum of all points squared)
		     f3 = sum( d2 .^ 2 )    (sum of all points squared)

		    note: -point refers to the value at a sub-sample
		          -the xmat is applied to d2, before calculating r
		              
@GLOBALS    : 
@CALLS      : 
@CREATED    : Feb 4, 1992 lc original in fit_vol
@MODIFIED   : Wed Jun 16 10:08:46 EST 1993 LC
        new code for minc tracc, copied from routines in fit_vol.

              Jun 23, 97 lc -
                 implement vox_space code so that comparisons computed at
                 each node of the 3D lattice use a single concatenated xform 
                 instead of v-w w-w w-v (three separate xforms.)

                 this yields a speed up of 30% for -xcorr - presumably
                 interpolation takes most of the time now.  This is
                 confirmed by running with the -nearest option.  On a
                 simple test volume,
                    orig w/-trilin = 73 secs (for 176 iters
                    new  w/-trilin = 47 secs (for 160 iters)
                    new  w/nearest = 18 secs (for 146 inters).
                    orig w/nearest = 30 secs (for 148 inters).
              Jun 27, 97 lc
                 instead of using the user-selected interpolant into the 1st volume,
                 I now force NN interpolation to the voxel nearest the 3D node. The coord
                 of this voxel is mapped through the concat'd transform into the 2nd
                 volume.
                    time = 38 sec. (160 iters)
                    w/-trilin
                 this is an overall speed up of 73/38 = 92%
                 the new version takes only     38/73 = 52%  of the time of the previous!
---------------------------------------------------------------------------- */
public float xcorr_objective(Volume d1,
			     Volume d2,
			     Volume m1,
			     Volume m2, 
			     Arg_Data *globals)
{

  VectorR
    vector_step;

  PointR
    starting_position,
    slice,
    row,
    col,
    pos2,
    voxel;

  double
    tx,ty,tz;
  int
    r,c,s;

  Real
    value1, value2;
  
  Real
    s1,s2,s3;                   /* to store the sums for f1,f2,f3 */
  float 
    result;				/* the result */
  int 
    count1,count2;


  Voxel_space_struct *vox_space;
  Transform          *trans;


                                /* prepare counters for this objective
                                   function */
  s1 = s2 = s3 = 0.0;
  count1 = count2 = 0;
                                /* prepare data for the voxel-to-voxel
                                   space transformation (instead of the
                                   general but inefficient world-world
                                   computations. */

  vox_space = new_voxel_space_struct();
  get_into_voxel_space(globals, vox_space, d1, d2);
  trans = get_linear_transform_ptr(vox_space->voxel_to_voxel_space);
  

                                /* loop through all nodes of the lattice */

  fill_Point( starting_position, vox_space->start[X], vox_space->start[Y], vox_space->start[Z]);

  /* ---------- step through all slices of lattice ------------- */
  for_less(s,0,globals->count[SLICE_IND]) { 

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    /* ---------- step through all rows of lattice ------------- */
    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );
      
      SCALE_POINT( col, row, 1.0); /* init first col position */

      /* ---------- step through all cols of lattice ------------- */
      for_less(c,0,globals->count[COL_IND]) {
		
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (nearest_neighbour_interpolant( d1, &voxel, &value1 )) {

	    count1++;

            my_homogenous_transform_point(trans,
                                          Point_x(voxel), Point_y(voxel), Point_z(voxel), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));

	 
	    fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
	    if (voxel_point_not_masked(m2, Point_x(pos2), Point_y(pos2), Point_z(pos2))) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {


		if (value1 > globals->threshold[0] && value2 > globals->threshold[1] ) {
		  
		  count2++;

		  s1 += value1*value2;
		  s2 += value1*value1;
		  s3 += value2*value2;
		  
		} 
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( col, col, vox_space->directions[COL_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */
  
  result = 1.0 - s1 / (sqrt((double)s2)*sqrt((double)s3));
  
  if (globals->flags.debug) (void)print ("%7d %7d -> %10.8f\n",count1,count2,result);

  delete_voxel_space_struct(vox_space);

  return (result);
  
}


public float ssc_objective(Volume d1,
			   Volume d2,
			   Volume m1,
			   Volume m2, 
			   Arg_Data *globals)
{
  VectorR
    vector_step;

  PointR
    starting_position,
    slice,
    row,
    col,
    pos2,
    voxel;

  double
    tx,ty,tz;
  int
    r,c,s;

  Real
    value1, value2;
  
  float 
    result;				/* the result */
  int 
    count1, count2,
    greater;
  unsigned  long
    zero_crossings;
  Voxel_space_struct *vox_space;
  Transform          *trans;

                                /* prepare counters for this objective
                                   function */
  greater = TRUE;
  zero_crossings = count1 = count2 = 0;

                                /* prepare data for the voxel-to-voxel
                                   space transformation (instead of the
                                   general but inefficient world-world
                                   computations. */

  vox_space = new_voxel_space_struct();
  get_into_voxel_space(globals, vox_space, d1, d2);
  trans = get_linear_transform_ptr(vox_space->voxel_to_voxel_space);

  fill_Point( starting_position, vox_space->start[X], vox_space->start[Y], vox_space->start[Z]);

  /* ------------------------  count along rows (fastest=col) first ------------------- */


  for_less(s,0,globals->count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );
      
      SCALE_POINT( col, row, 1.0); /* init first col position */
      for_less(c,0,globals->count[COL_IND]) {
	
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;

            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));

	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
	    if (voxel_point_not_masked(m2, Point_x(pos2), Point_y(pos2), Point_z(pos2))) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;

		if (!((greater && value1>value2) || (!greater && value1<value2))) {
		  greater = !greater;
		  zero_crossings++;
		} 
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( col, col, vox_space->directions[COL_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */

  /* ------------------------  count along cols second  --(fastest=row)--------------- */

  for_less(s,0,globals->count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_less(c,0,globals->count[COL_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[COL_IND], c);
      ADD_POINT_VECTOR( col, slice, vector_step );
      
      SCALE_POINT( row, col, 1.0); /* init first row position */

      for_less(r,0,globals->count[ROW_IND]) {

                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;

            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));
	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
	    if (voxel_point_not_masked(m2,Point_x(pos2), Point_y(pos2), Point_z(pos2))) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;

		if (!((greater && value1>value2) || (!greater && value1<value2))) {
		  greater = !greater;
		  zero_crossings++;
		} 
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( row, row, vox_space->directions[ROW_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */




  /* ------------------------  count along slices last ------------------------ */


  for_less(c,0,globals->count[COL_IND]) {
    
    SCALE_VECTOR( vector_step, vox_space->directions[COL_IND], c);
    ADD_POINT_VECTOR( col, starting_position, vector_step );

    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, col, vector_step );
      
      SCALE_POINT( slice, row, 1.0); /* init first col position */

      for_less(s,0,globals->count[SLICE_IND]) {
	
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;

            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));
	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */

	    if (voxel_point_not_masked(m2, Point_x(pos2), Point_y(pos2), Point_z(pos2))) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;

		if (!((greater && value1>value2) || (!greater && value1<value2))) {
		  greater = !greater;
		  zero_crossings++;
		} 
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( slice, slice, vox_space->directions[SLICE_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */


  result = -1.0 * (float)zero_crossings;

  if (globals->flags.debug) (void)print ("%7d %7d -> %10.8f\n",count1,count2,result);

  return (result);
  
}

public float zscore_objective(Volume d1,
			   Volume d2,
			   Volume m1,
			   Volume m2, 
			   Arg_Data *globals)
{
  VectorR
    vector_step;

  PointR 
    starting_position,
    slice,
    row,
    col,
    pos2,
    voxel;

  double
    tx,ty,tz;
  int
    r,c,s;

  Real
    value1, value2;
  
  Real
    z2_sum;
  float 
    result;				/* the result */
  int 
    count1,count2,count3;
  Voxel_space_struct *vox_space;
  Transform          *trans;


                                /* prepare data for the voxel-to-voxel
                                   space transformation (instead of the
                                   general but inefficient world-world
                                   computations. */

  vox_space = new_voxel_space_struct();
  get_into_voxel_space(globals, vox_space, d1, d2);
  trans = get_linear_transform_ptr(vox_space->voxel_to_voxel_space);


  fill_Point( starting_position, vox_space->start[X], vox_space->start[Y], vox_space->start[Z]);

  z2_sum = 0.0;
  count1 = count2 = count3 = 0;

  for_less(s,0,globals->count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );
      
      SCALE_POINT( col, row, 1.0); /* init first col position */
      for_less(c,0,globals->count[COL_IND]) {
	
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;

            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));
	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
	    if (voxel_point_not_masked(m2, Point_x(pos2), Point_y(pos2), Point_z(pos2))) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;

		if (ABS(value1) > globals->threshold[0] && ABS(value2) > globals->threshold[1] ) {
		  count3++;
		  z2_sum +=  (value1-value2)*(value1-value2);
		} 
	
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( col, col, vox_space->directions[COL_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */

  if (count3 > 0)
    result = sqrt((double)z2_sum) / count3;
  else
    result = sqrt((double)z2_sum);

  if (globals->flags.debug) (void)print ("%7d %7d %7d -> %10.8f\n",count1,count2,count3,result);
  
  return (result);
  
}



public float vr_objective(Volume d1,
			  Volume d2,
			  Volume m1,
			  Volume m2, 
			  Arg_Data *globals)
{
  VectorR
    vector_step;

  PointR
    starting_position,
    slice,
    row,
    col,
    pos2,
    voxel;

  double
    tx,ty,tz;
  int
    r,c,s;

  Real
    value1, value2,
    voxel_value1;
  
  float
    rat,
    total_variance,
    *limits,
    *rat_sum,
    *rat2_sum,
    *var;
  unsigned long
    total_count,
    *count3;

  float 
    result;				/* the result */
  int 
    index,i,count1,count2;
  Voxel_space_struct *vox_space;
  Transform          *trans;



				/* build segmentation info!  */

  ALLOC(rat_sum  ,1+segment_table->groups);
  ALLOC(rat2_sum ,1+segment_table->groups);
  ALLOC(var      ,1+segment_table->groups);
  ALLOC(limits   ,1+segment_table->groups);

  ALLOC(count3, (segment_table->groups+1));

                                /* prepare data for the voxel-to-voxel
                                   space transformation (instead of the
                                   general but inefficient world-world
                                   computations. */

  vox_space = new_voxel_space_struct();
  get_into_voxel_space(globals, vox_space, d1, d2);
  trans = get_linear_transform_ptr(vox_space->voxel_to_voxel_space);



  fill_Point( starting_position, vox_space->start[X], vox_space->start[Y], vox_space->start[Z]);

				/* init running sums and counters. */
  for_inclusive( i, 1, segment_table->groups) {
    rat_sum[i] = 0.0;
    rat2_sum[i] = 0.0;
    count3[i]   = 0;
    var[i] = 0.0;
  }
  count1 = count2 = 0;

				/* loop through each node of lattice */
  for_less(s,0,globals->count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );
      
      SCALE_POINT( col, row, 1.0); /* init first col position */
      for_less(c,0,globals->count[COL_IND]) {
	
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;
	    voxel_value1 = CONVERT_VALUE_TO_VOXEL(d1,value1 );


            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));
	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
	    if (voxel_point_not_masked(m2,Point_x(pos2), Point_y(pos2), Point_z(pos2) )) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;
		/* voxel_value2 = CONVERT_VALUE_TO_VOXEL(d1,value2 ); */

	        if (value1 > globals->threshold[0] && value2 > globals->threshold[1]
		    && value2 != 0.0)  {

		  index = (*segment_table->segment)( voxel_value1, segment_table);

		  if (index>0) {
		    count3[index]++;
		    rat = value1 / value2;
		    rat_sum[index] += rat;
		    rat2_sum[index] +=  rat*rat;
		  }
		  else {
		    print_error_and_line_num("Cannot segment voxel value %d into one of %d groups.", 
				__FILE__, __LINE__, voxel_value1,segment_table->groups );
		    exit(EXIT_FAILURE);

		  }
		} 
		
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( col, col, vox_space->directions[COL_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */


  total_variance = 0.0;
  total_count = 0;

  for_inclusive( index, 1, segment_table->groups) {
    if (count3[index] > 1) 
      total_count += count3[index];
  }

  if (total_count > 1) {
    for_inclusive( index, 1, segment_table->groups) {
      if (count3[index] > 1) {
	var[index]  = ((double)count3[index]*rat2_sum[index] - rat_sum[index]*rat_sum[index]) / 
	  ((double)count3[index]*((double)count3[index]-1.0));
	
	total_variance += ((double)count3[index]/(double)total_count) * var[index];
      }
      else
	var[index] = 0.0;
    }
  }
  else
    total_variance = 1e15;

  result = total_variance;

  if (globals->flags.debug) print ("%7d %7d %7d -> %10.8f\n",count1,count2,count3[1],result);

  FREE(rat_sum);
  FREE(rat2_sum);
  FREE(var);
  FREE(limits);
  FREE(count3);



  return (result);
  
}

public float stub_objective(Volume d1,
			    Volume d2,
			    Volume m1,
			    Volume m2, 
			    Arg_Data *globals)
{

  VectorR			/* these variables are used to step through */
    vector_step;		/* the 3D lattice                           */
  PointR
    starting_position,
    slice,
    row,
    col,
    pos2,
    voxel;
  double
    tx,ty,tz;
  int
    r,c,s;
  Real
    value1, value2,
    voxel_value1;
  float 
    result;				/* the result */
  int 
    count1,count2;		/* number of nodes in first vol, second vol */
  Voxel_space_struct *vox_space;
  Transform          *trans;



				/* init any objective function specific
				   stuff here                           */
  count1 = count2 = 0;
  result = 0.0;

                                /* prepare data for the voxel-to-voxel
                                   space transformation (instead of the
                                   general but inefficient world-world
                                   computations. */

  vox_space = new_voxel_space_struct();
  get_into_voxel_space(globals, vox_space, d1, d2);
  trans = get_linear_transform_ptr(vox_space->voxel_to_voxel_space);

			/* build world lattice info */

  fill_Point( starting_position, vox_space->start[X], vox_space->start[Y], vox_space->start[Z]);

				/* loop through each node of lattice */
  for_less(s,0,globals->count[SLICE_IND]) {

    SCALE_VECTOR( vector_step, vox_space->directions[SLICE_IND], s);
    ADD_POINT_VECTOR( slice, starting_position, vector_step );

    for_less(r,0,globals->count[ROW_IND]) {
      
      SCALE_VECTOR( vector_step, vox_space->directions[ROW_IND], r);
      ADD_POINT_VECTOR( row, slice, vector_step );
      
      SCALE_POINT( col, row, 1.0); /* init first col position */
      for_less(c,0,globals->count[COL_IND]) {
	
                                /* use the voxel center closest to this lattice
                                   node. 
                                */
	fill_Point( voxel, ROUND(Point_x(col)), ROUND(Point_y(col)), ROUND(Point_z(col)) ); 
	
				/* get the node value in volume 1,
				   if it falls within the volume    */

	if (voxel_point_not_masked(m1, Point_x(voxel), Point_y(voxel), Point_z(voxel))) {
	  
	  if (INTERPOLATE_TRUE_VALUE( d1, &voxel, &value1 )) {

	    count1++;
	    voxel_value1 = CONVERT_VALUE_TO_VOXEL(d1,value1 );

				/* transform the node coordinate into
				   volume 2                             */

            my_homogenous_transform_point(trans,
                                          Point_x(col), Point_y(col), Point_z(col), 1.0,
                                          &Point_x(pos2), &Point_y(pos2), &Point_z(pos2));
	    
            fill_Point( voxel, Point_x(pos2), Point_y(pos2), Point_z(pos2) ); /* build the voxel POINT */
	
				/* get the node value in volume 2,
				   if it falls within the volume    */

	    if (voxel_point_not_masked(m2,Point_x(pos2), Point_y(pos2), Point_z(pos2) )) {
	      
	      if (INTERPOLATE_TRUE_VALUE( d2, &voxel, &value2 )) {

		count2++;
		/* voxel_value2 = CONVERT_VALUE_TO_VOXEL(d1,value2 ); */


				/* if both values are within the threshold
				   limits, then do what is necessary to
				   calculate the objective function         */

	        if (ABS(value1) > globals->threshold[0] && 
		    ABS(value2) > globals->threshold[1] ) {

				/*EMPTY*/
				/* EMPTY is for lint  */
				/* calculate objective function data here */


		} /* if value1 & value2 > threshold */
		
		
	      } /* if voxel in d2 */
	    } /* if point in mask volume two */
	  } /* if voxel in d1 */
	} /* if point in mask volume one */
	
	ADD_POINT_VECTOR( col, col, vox_space->directions[COL_IND] );
	
      } /* for c */
    } /* for r */
  } /* for s */


  /* now that the data for the objective function has been accumulated
     over the lattice nodes, finish the objective function calculation, 
     placing the final objective function value in  'result' */


  /* don't forget to free up any variables you declared above */

  return (result);
  
}





