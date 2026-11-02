#pragma once
namespace diagnostic {
enum class WeaponKind {Other,OneHand,TwoHand,Staff,Bow,Crossbow,Shield,Arrow,Bolt};
inline WeaponKind WeaponType(int type){switch(type){
case 2:return WeaponKind::OneHand;case 3:case 52:return WeaponKind::TwoHand;
case 12:case 51:return WeaponKind::Staff;
case 5:return WeaponKind::Bow;case 6:return WeaponKind::Crossbow;
case 9:return WeaponKind::Shield;case 4:return WeaponKind::Arrow;case 7:return WeaponKind::Bolt;
default:return WeaponKind::Other;}}
inline bool PhysicalMeleeType(int type){auto kind=WeaponType(type);return kind==WeaponKind::OneHand||kind==WeaponKind::TwoHand||kind==WeaponKind::Staff;}
inline bool TwoHandType(int type){return WeaponType(type)==WeaponKind::TwoHand||WeaponType(type)==WeaponKind::Staff;}
// Render profile only: native inventory, skills and damage retain the real type.
inline int MeleePoseType(int nativeType){return WeaponType(nativeType)==WeaponKind::Staff?12:nativeType;}
}
