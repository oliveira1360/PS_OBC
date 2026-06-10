// Wrapper tipado sobre a satellite.js (carregada via CDN em index.html, global `satellite`).
import type { GroundStation } from '../api/orbit'

// A lib é exposta como global; declaramos a forma mínima que usamos.
declare global {
  interface Window { satellite?: any }
}

const sat = (): any => {
  const s = window.satellite
  if (!s) throw new Error('Biblioteca de propagação orbital (satellite.js) não carregada')
  return s
}

export interface SubPoint { lat: number; lon: number; altKm: number }
export interface TrackPoint { lat: number; lon: number }
export interface Pass {
  aos: Date
  los: Date
  maxElevation: number
  durationSec: number
}

const DEG = 180 / Math.PI
const RAD = Math.PI / 180

export function isOrbitReady(): boolean {
  return !!window.satellite
}

export function makeSatrec(line1: string, line2: string): any {
  return sat().twoline2satrec(line1.trim(), line2.trim())
}

/** Sub-ponto (lat/lon/alt) do satélite num instante. */
export function subPoint(satrec: any, date: Date): SubPoint | null {
  const s = sat()
  const pv = s.propagate(satrec, date)
  if (!pv || !pv.position) return null
  const gmst = s.gstime(date)
  const geo = s.eciToGeodetic(pv.position, gmst)
  return {
    lat: s.degreesLat(geo.latitude),
    lon: s.degreesLong(geo.longitude),
    altKm: geo.height,
  }
}

/** Ground track ao longo de `minutes` a partir de `from`, com passos de `stepSec`. */
export function groundTrack(satrec: any, from: Date, minutes = 100, stepSec = 30): TrackPoint[] {
  const out: TrackPoint[] = []
  const steps = Math.floor((minutes * 60) / stepSec)
  for (let i = 0; i <= steps; i++) {
    const d = new Date(from.getTime() + i * stepSec * 1000)
    const sp = subPoint(satrec, d)
    if (sp) out.push({ lat: sp.lat, lon: sp.lon })
  }
  return out
}

/** Elevação (graus) do satélite visto da estação num instante. */
function elevationAt(satrec: any, station: GroundStation, date: Date): number | null {
  const s = sat()
  const pv = s.propagate(satrec, date)
  if (!pv || !pv.position) return null
  const gmst = s.gstime(date)
  const ecf = s.eciToEcf(pv.position, gmst)
  const observer = {
    longitude: station.longitude * RAD,
    latitude: station.latitude * RAD,
    height: station.altitudeM / 1000,
  }
  const look = s.ecfToLookAngles(observer, ecf)
  return look.elevation * DEG
}

/** Próximas passagens visíveis (elevação > 0) nas próximas `hours`. */
export function nextPasses(
  satrec: any,
  station: GroundStation,
  from: Date,
  hours = 48,
  stepSec = 30,
  maxPasses = 8
): Pass[] {
  const passes: Pass[] = []
  const total = Math.floor((hours * 3600) / stepSec)
  let inPass = false
  let aos: Date | null = null
  let maxEl = -90

  for (let i = 0; i <= total; i++) {
    const d = new Date(from.getTime() + i * stepSec * 1000)
    const el = elevationAt(satrec, station, d)
    if (el == null) continue
    if (el > 0) {
      if (!inPass) { inPass = true; aos = d; maxEl = el }
      else if (el > maxEl) maxEl = el
    } else if (inPass && aos) {
      passes.push({
        aos, los: d, maxElevation: maxEl,
        durationSec: (d.getTime() - aos.getTime()) / 1000,
      })
      inPass = false; aos = null; maxEl = -90
      if (passes.length >= maxPasses) break
    }
  }
  return passes
}
