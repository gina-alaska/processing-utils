   require 'rake/version_task'
   Rake::VersionTask.new do |task|
     task.with_git_tag = true
   end

  task :default => [:build]

  task :build => [:check_gdal, :build_cc_stuff]

  task :check_gdal do
    gdal = `which gdal-config`
    raise ("Gdal not found - are you sure your enviroment is setup properly?") if (gdal=="")
    gdal_version =  `gdal-config --version`
    raise ("Your gdal version is pretty old (#{gdal_version}) - get a new one") if (gdal_version<"1.10.0")
  end

  task :build_cc_stuff => [:add_mask, :masker,:no_data_check, :get_gcp, :modis_natural_color_stretch, :image_info, :npp_natural_color_stretch, :awips_thermal_stretch, :awips_vis_stretch, :sqrt_stretch] 

  task :add_mask do
    puts("Building \"add_mask\"")
    system("gcc $(gdal-config --cflags) -o bin/add_mask src/add_mask.c $(gdal-config --libs)")
  end

  task :masker do
    puts("Building \"masker\"")
    system("gcc $(gdal-config --cflags) -o bin/masker src/masker.c $(gdal-config --libs)")
  end

  task :no_data_check do
    puts("Building \"no_data_check\"")
    system("gcc $(gdal-config --cflags) -o  bin/no_data_check src/no_data_check.c $(gdal-config --libs)")
  end

  task :git_pull do
    system("git pull origin master")
  end


  task :get_gcp do
    puts("Building \"get cgp\"")
    system("gcc -g $(gdal-config --cflags) -o bin/get_gcp src/get_gcp.c $(gdal-config --libs)")
  end

  task :modis_natural_color_stretch do
    puts("Building \":modis_natural_color_stretch\"")
    system("gcc $(gdal-config --cflags) -o bin/modis_natural_color_stretch src/modis_natural_color_stretch.c $(gdal-config --libs)")
  end

  task :image_info  do
    puts("Building image_info..")
    system("g++ -Wno-conversion-null  $(gdal-config --cflags) -o bin/image_info src/image_info.cpp $(gdal-config --libs)")
  end

  task :npp_natural_color_stretch do
    system(" gcc -O3  $(gdal-config --cflags) -o bin/npp_natural_color_stretch src/npp_natural_color_stretch.c $(gdal-config --libs)")
  end
  
  task :awips_thermal_stretch do 
    system(" gcc -O3  $(gdal-config --cflags) -o bin/awips_thermal_stretch src/awips_thermal_stretch.c $(gdal-config --libs) -lm")
  end
  task :awips_vis_stretch do
    system(" gcc -O3  $(gdal-config --cflags) -o bin/awips_vis_stretch src/awips_vis_stretch.c $(gdal-config --libs) -lm")
  end

  task :sqrt_stretch do
    system(" gcc -O3  $(gdal-config --cflags) -o bin/sqrt_stretch src/sqrt_stretch.c $(gdal-config --libs) -lm")
  end


