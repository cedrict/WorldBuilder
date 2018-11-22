/*
  Copyright (C) 2018 by the authors of the World Builder code.

  This file is part of the World Builder.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <boost/algorithm/string.hpp>

#include <world_builder/features/continental_plate.h>
#include <world_builder/features/continental_plate_models/temperature/interface.h>
#include <world_builder/features/continental_plate_models/composition/interface.h>
#include <world_builder/utilities.h>
#include <world_builder/assert.h>
#include <world_builder/nan.h>
#include <world_builder/parameters.h>

#include <world_builder/types/array.h>
#include <world_builder/types/constant_layer.h>
#include <world_builder/types/double.h>
#include <world_builder/types/string.h>
#include <world_builder/types/unsigned_int.h>
#include <world_builder/types/plugin_system.h>


namespace WorldBuilder
{
  using namespace Utilities;

  namespace Features
  {
    ContinentalPlate::ContinentalPlate(WorldBuilder::World *world_)
      :
      temperature_submodule_constant_depth(NaN::DSNAN),
      temperature_submodule_constant_temperature(NaN::DSNAN),
      composition_submodule_constant_depth(NaN::DSNAN),
      composition_submodule_constant_composition(NaN::ISNAN)
    {
      this->world = world_;
      this->name = "continental plate";
    }

    ContinentalPlate::~ContinentalPlate()
    { }


    void
    ContinentalPlate::declare_entries(Parameters &prm, const std::string &)
    {
      prm.declare_entry("min depth", "0", false, Types::Double(0),
                        "The depth to which this feature is present");
      prm.declare_entry("max depth", "0", true, Types::Double(std::numeric_limits<double>::max()),
                        "The depth to which this feature is present");
      prm.declare_entry("temperature models","",true,
                        Types::PluginSystem(Features::ContinentalPlateModels::Temperature::Interface::declare_entries),
                        "A list of temperature models.");
      prm.declare_entry("composition models","",true,
                        Types::PluginSystem(Features::ContinentalPlateModels::Composition::Interface::declare_entries),
                        "A list of composition models.");
    }

    void
    ContinentalPlate::parse_entries(Parameters &prm)
    {
      const CoordinateSystem coordinate_system = prm.coordinate_system->natural_coordinate_system();

      this->name = prm.get<std::string>("name");
      this->coordinates = prm.get_vector<Point<2> >("coordinates");
      if (coordinate_system == CoordinateSystem::spherical)
        for (auto &coordinate: coordinates)
          coordinate *= const_pi / 180.0;

      min_depth = prm.get<double>("min depth");
      max_depth = prm.get<double>("max depth");


      prm.get_unique_pointers<Features::ContinentalPlateModels::Temperature::Interface>("temperature models", temperature_models);

      prm.enter_subsection("temperature models");
      {
        for (unsigned int i = 0; i < temperature_models.size(); ++i)
          {
            prm.enter_subsection(std::to_string(i));
            {
              temperature_models[i]->parse_entries(prm);
            }
            prm.leave_subsection();
          }
      }
      prm.leave_subsection();


      prm.get_unique_pointers<Features::ContinentalPlateModels::Composition::Interface>("composition models", composition_models);

      prm.enter_subsection("composition models");
      {
        for (unsigned int i = 0; i < composition_models.size(); ++i)
          {
            prm.enter_subsection(std::to_string(i));
            {
              composition_models[i]->parse_entries(prm);
            }
            prm.leave_subsection();
          }
      }
      prm.leave_subsection();

      //this->parse_interface_entries(prm, coordinate_system);
      /*
            prm.enter_subsection("temperature model");
            {

              //prm.get_unique_pointers(temperature_submodule_name, )
              prm.load_entry("name", true, Types::String("","The name of the temperature model."));
              temperature_submodule_name = prm.get_string("name");

              if (temperature_submodule_name == "constant")
                {
                  prm.load_entry("depth", true, Types::Double(NaN::DSNAN,"The depth in meters to which the temperature of this feature is present."));
                  temperature_submodule_constant_depth = prm.get_double("depth");

                  prm.load_entry("temperature", true, Types::Double(0,"The temperature in degree Kelvin which this feature should have"));
                  temperature_submodule_constant_temperature = prm.get_double("temperature");
                }
              else if (temperature_submodule_name == "linear")
                {
                  prm.load_entry("depth", true, Types::Double(NaN::DSNAN,"The depth in meters to which the temperature rises (or lowers) to."));
                  temperature_submodule_linear_depth = prm.get_double("depth");

                  prm.load_entry("top temperature", false, Types::Double(293.15,
                                                                         "The temperature in degree Kelvin at the top of this block."));
                  temperature_submodule_linear_top_temperature = prm.get_double("top temperature");


                  prm.load_entry("bottom temperature", false, Types::Double(NaN::DQNAN,"The temperature in degree Kelvin a the bottom of this block. "
                                                                            "If this value is not defined, the adiabatic temperature at this depth is used."));
                  temperature_submodule_linear_bottom_temperature = prm.get_double("bottom temperature");
                }

            }
            prm.leave_subsection();

            prm.enter_subsection("composition model");
            {
              prm.load_entry("name", true, Types::String("","The name of the composition model used."));
              composition_submodule_name = prm.get_string("name");

              if (composition_submodule_name == "constant")
                {
                  prm.load_entry("depth", true,
                                 Types::Double(NaN::DSNAN,"The depth in meters to which the composition of this feature is present."));
                  composition_submodule_constant_depth = prm.get_double("depth");

                  prm.load_entry("compositions", true,
                                 Types::Array(Types::UnsignedInt(0,"The number of the composition that is present there."),
                                              "A list of compositions which are present"));
                  std::vector<Types::UnsignedInt> temp_composition = prm.get_array<Types::UnsignedInt>("compositions");
                  composition_submodule_constant_composition.resize(temp_composition.size());
                  for (unsigned int i = 0; i < temp_composition.size(); ++i)
                    {
                      composition_submodule_constant_composition[i] = temp_composition[i].value;
                    }

                  prm.load_entry("fractions", false,
                                 Types::Array(Types::Double(1.0,"The value between 0 and 1 of how much this composition is present."),
                                              "A list of compositional fractions corresponding to the compositions list."));
                  std::vector<Types::Double> temp_fraction = prm.get_array<Types::Double>("fractions");
                  composition_submodule_constant_value.resize(temp_fraction.size());
                  for (unsigned int i = 0; i < temp_composition.size(); ++i)
                    {
                      composition_submodule_constant_value[i] = temp_fraction[i].value;
                    }

                  WBAssertThrow(composition_submodule_constant_composition.size() == composition_submodule_constant_value.size(),
                                "There are not the same amount of compositions and fractions.");
                }
              else if (composition_submodule_name == "constant layers")
                {
                  // Load the layers.
                  prm.load_entry("layers", true, Types::Array(Types::ConstantLayer({NaN::ISNAN}, {1.0},NaN::DSNAN,
                                                                                   "A plate constant layer with a certain composition and thickness."),
                                                              "A list of layers."));

                  std::vector<Types::ConstantLayer> constant_layers = prm.get_array<Types::ConstantLayer>("layers");

                  composition_submodule_constant_layers_compositions.resize(constant_layers.size());
                  composition_submodule_constant_layers_thicknesses.resize(constant_layers.size());
                  composition_submodule_constant_layers_value.resize(constant_layers.size());

                  for (unsigned int i = 0; i < constant_layers.size(); ++i)
                    {
                      composition_submodule_constant_layers_compositions[i] = constant_layers[i].value_composition;
                      composition_submodule_constant_layers_thicknesses[i] = constant_layers[i].value_thickness;
                      composition_submodule_constant_layers_value[i] = constant_layers[i].value;
                    }
                }
              else
                {
                  WBAssertThrow(composition_submodule_name == "none","Subducting plate temperature model '" << temperature_submodule_name << "' not found.");
                }
            }
            prm.leave_subsection();*/
    }


    double
    ContinentalPlate::temperature(const Point<3> &position,
                                  const double depth,
                                  const double gravity_norm,
                                  double temperature) const
    {
      WorldBuilder::Utilities::NaturalCoordinate natural_coordinate = WorldBuilder::Utilities::NaturalCoordinate(position,
                                                                      *(world->parameters.coordinate_system));

      if (depth <= max_depth && depth >= min_depth &&
          Utilities::polygon_contains_point(coordinates, Point<2>(natural_coordinate.get_surface_coordinates(),
                                                                  world->parameters.coordinate_system->natural_coordinate_system())))
        {
          for (auto &temperature_model: temperature_models)
            {
              temperature = temperature_model->get_temperature(position,
                                                               depth,
                                                               gravity_norm,
                                                               temperature);
            }
        }
      /*if (temperature_submodule_name == "constant")
        {

          // The constant temperature module should be used for this.
          if (depth <= temperature_submodule_constant_depth &&
              Utilities::polygon_contains_point(coordinates, Point<2>(natural_coordinate.get_surface_coordinates(),
                                                                      world->parameters.coordinate_system->natural_coordinate_system())))
            {
              // We are in the the area where the contintal plate is defined. Set the constant temperature.
              return temperature_submodule_constant_temperature;
            }
        }
      else if (temperature_submodule_name == "linear")
        {
          WorldBuilder::Utilities::NaturalCoordinate natural_coordinate = WorldBuilder::Utilities::NaturalCoordinate(position,
                                                                          *(world->parameters.coordinate_system));

          // The linear temperature module should be used for this.
          if (depth <= temperature_submodule_linear_depth &&
              Utilities::polygon_contains_point(coordinates, Point<2>(natural_coordinate.get_surface_coordinates(),
                                                                      world->parameters.coordinate_system->natural_coordinate_system())))
            {
              double bottom_temperature = temperature_submodule_linear_bottom_temperature;
              if (std::isnan(temperature_submodule_linear_bottom_temperature))
                {
                  bottom_temperature =  this->world->parameters.get_double("potential mantle temperature") *
                                        std::exp(((this->world->parameters.get_double("thermal expansion coefficient") * gravity_norm) /
                                                  this->world->parameters.get_double("specific heat")) * depth);
                }

              return temperature_submodule_linear_top_temperature +
                     depth * ((bottom_temperature - temperature_submodule_linear_top_temperature) / temperature_submodule_linear_depth);
            }


        }
      else if (temperature_submodule_name == "none")
        {
          return temperature;
        }
      else
        {
          WBAssertThrow(false,"Given temperature module does not exist: " + temperature_submodule_name);
        }*/

      return temperature;
    }

    double
    ContinentalPlate::composition(const Point<3> &position,
                                  const double depth,
                                  const unsigned int composition_number,
                                  double composition) const
    {
      WorldBuilder::Utilities::NaturalCoordinate natural_coordinate = WorldBuilder::Utilities::NaturalCoordinate(position,
                                                                      *(world->parameters.coordinate_system));

      if (depth <= max_depth && depth >= min_depth &&
          Utilities::polygon_contains_point(coordinates, Point<2>(natural_coordinate.get_surface_coordinates(),
                                                                  world->parameters.coordinate_system->natural_coordinate_system())))
        {
          for (auto &composition_model: composition_models)
            {
              composition = composition_model->get_composition(position,
                                                               depth,
                                                               composition_number,
                                                               composition);
            }
        }
      /*if (composition_submodule_name == "constant")
        {
          WorldBuilder::Utilities::NaturalCoordinate natural_coordinate = WorldBuilder::Utilities::NaturalCoordinate(position,*(world->parameters.coordinate_system));
          // The constant temperature module should be used for this.
          if (depth <= composition_submodule_constant_depth &&
              Utilities::polygon_contains_point(coordinates, Point<2>(natural_coordinate.get_surface_coordinates(),world->parameters.coordinate_system->natural_coordinate_system())))
            {
              // We are in the the area where the contintal plate is defined. Set the constant temperature.
              const bool clear = true;
              for (unsigned int i =0; i < composition_submodule_constant_composition.size(); ++i)
                {
                  if (composition_submodule_constant_composition[i] == composition_number)
                    {
                      return composition_submodule_constant_value[i];
                    }
                  else if (clear == true)
                    {
                      composition = 0.0;
                    }
                }
            }

        }
      else if (composition_submodule_name == "constant layers")
        {
          // find out what layer we are in.
          double total_thickness = 0;
          for (unsigned int i = 0; i < composition_submodule_constant_layers_compositions.size(); ++i)
            {
              WorldBuilder::Utilities::NaturalCoordinate natural_coordinate = WorldBuilder::Utilities::NaturalCoordinate(position,*(world->parameters.coordinate_system));

              // Check whether we are in the correct layer
              if (depth >= total_thickness
                  && depth < total_thickness + composition_submodule_constant_layers_thicknesses[i]
                  && Utilities::polygon_contains_point(coordinates,
                                                       Point<2>(natural_coordinate.get_surface_coordinates(),
                                                                world->parameters.coordinate_system->natural_coordinate_system())))
                {
                  // We are in a layer. Check whether this is the correct composition.
                  // The composition_number is cast to an int to prevent a warning.
                  // The reason composition_submodule_constant_layers_compositions is
                  // unsigned int is so that it can be set to a negative value, which
                  // is always ignored.
                  const bool clear = true;
                  for (unsigned int j =0; j < composition_submodule_constant_layers_compositions[i].size(); ++j)
                    {
                      if (composition_submodule_constant_layers_compositions[i][j] == composition_number)
                        {
                          return composition_submodule_constant_layers_value[i][j];
                        }
                      else if (clear == true)
                        {
                          composition = 0.0;
                        }
                    }
                }
              total_thickness += composition_submodule_constant_layers_thicknesses[i];
            }
        }
      else if (composition_submodule_name == "none")
        {
          return composition;
        }
      else
        {
          WBAssertThrow(false,"Given composition module does not exist: " + composition_submodule_name);
        }*/

      return composition;
    }

    WB_REGISTER_FEATURE(ContinentalPlate, continental plate)

  }
}
