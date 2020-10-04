#ifndef ENTITY_H
#define ENTITY_H

#include <functional>

#include "render/renderelement.h"
#include "physics/physicsobject.h"

#include "node/node.h"

class Entity;

typedef std::function<std::shared_ptr<Entity>(std::shared_ptr<strc::Node>, std::shared_ptr<PhysicsObject>)> EntityBuilder;

class Entity
{
public:
  Entity(std::shared_ptr<strc::Node> node, std::shared_ptr<PhysicsObject> physObject);
  //Entity(std::shared_ptr<RenderElement> renderElement, RenderElement::Instance * instance, std::shared_ptr<PhysicsObject> physObject);
  virtual ~Entity();

  virtual void onCollision(std::shared_ptr<Entity> entity, double impulse, double force);
  virtual void onUpdate(const double dt);
  void synchronize();

  void applyImpulse(Math::Vector<3, double> impulse);
  void applyForce(Math::Vector<3, double> force, Math::Vector<3, double> pos);
  double getMass();

  Math::Vector<3,double> getPosition();

  std::shared_ptr<PhysicsObject> getPhysicsObject();

  static void registerEntityType(std::string type, EntityBuilder builder);
  static void registerDefaultEntityTypes();
  static std::shared_ptr<Entity> buildEntityFromType(std::string type, std::shared_ptr<strc::Node> node, std::shared_ptr<PhysicsObject> physObject);

protected:

private:

  bool isStatic;

  std::shared_ptr<strc::Node> node;
  //std::shared_ptr<RenderElement> renderElement;
  //RenderElement::Instance renderInstance;
  std::shared_ptr<PhysicsObject> physObject;

  static std::unordered_map<std::string, EntityBuilder> builders;


};

#endif // ENTITY_H
